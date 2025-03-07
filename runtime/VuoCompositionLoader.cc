/**
 * @file
 * VuoCompositionLoader implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <dlfcn.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include "VuoTelemetry.h"

void *ZMQLoaderControlContext = NULL;  ///< The context for initializing sockets to control the composition loader.
void *ZMQLoaderControl = NULL;  ///< The socket for controlling the composition loader.
void *ZMQControlContext = NULL;  ///< The context for initializing sockets to control the composition.
void *ZMQControl = NULL;  ///< The socket for controlling the composition.
char *controlURL = NULL;  ///< The URL that the composition will use to initialize its control socket.
char *telemetryURL = NULL;  ///< The URL that the composition will use to initialize its telemetry socket.
bool *isStopped = NULL;  ///< True if the composition has stopped.
bool *isPaused = NULL;  ///< True if the composition is paused.
bool isReplacing = false;  ///< True if the composition is in the process of being replaced.
bool isReferenceCountingInitialized = false;  ///< True if VuoHeap_init() has been called.
void *dylibHandle = NULL;  ///< A handle to the running composition.
void **resourceDylibHandles = NULL;  ///< A list of handles to the running composition's resources.
size_t resourceDylibHandlesSize = 0;  ///< The number of items in @c resourceDylibHandles.
size_t resourceDylibHandlesCapacity = 0;  ///< The number of items that @c resourceDylibHandlesCapacity can currently hold.
pid_t runnerPid = 0;  ///< Process ID of the runner that started the composition.

void replaceComposition(const char *dylibPath, const char *compositionDiff);
void stopComposition(void);
void pauseComposition(void);
void unpauseComposition(void);
void loadResourceDylib(const char *resourceDylibPath);
void unloadResourceDylibs(void);

/**
 * Sends a control reply message to the process controlling this composition loader.
 */
void vuoLoaderControlReplySend(enum VuoLoaderControlReply reply, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoLoaderControl",ZMQLoaderControl,reply,messages,messageCount,false);
}

/**
 * Sends a control request message to the running composition.
 */
void vuoControlRequestSend(enum VuoControlRequest request, zmq_msg_t *messages, unsigned int messageCount)
{
	vuoSend("VuoControl",ZMQControl,request,messages,messageCount,false);
}

/**
 * Receives a control reply message from the running composition, and checks that it is the expected reply.
 */
void vuoControlReplyReceive(enum VuoControlReply expectedReply)
{
	int reply = vuoReceiveInt(ZMQControl);
	if (reply != expectedReply)
		VLog("Error: VuoControl message received unexpected reply (received %d, expected %d).", reply, expectedReply);
}

/**
 * Starts the composition loader.
 */
int main(int argc, char **argv)
{
	char *loaderControlURL = NULL;
	bool isPausedOnStart = false;
	isPaused = (bool *)malloc(sizeof(bool));
	*isPaused = isPausedOnStart;
	runnerPid = getppid();

	// Parse commandline arguments.
	{
		static struct option options[] = {
			{"vuo-control", required_argument, NULL, 0},
			{"vuo-telemetry", required_argument, NULL, 0},
			{"vuo-pause", no_argument, NULL, 0},
			{"vuo-loader", required_argument, NULL, 0},
			{"vuo-runner", required_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		while((getopt_long(argc, argv, "", options, &optionIndex)) != -1)
		{
			switch(optionIndex)
			{
				case 0:	// "vuo-control"
					controlURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(controlURL, optarg);
					break;
				case 1:	// "vuo-telemetry"
					telemetryURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(telemetryURL, optarg);
					break;
				case 2: // "vuo-pause"
					*isPaused = isPausedOnStart = true;
					break;
				case 3:	// "vuo-loader"
					if (loaderControlURL)
						free(loaderControlURL);
					loaderControlURL = (char *)malloc(strlen(optarg) + 1);
					strcpy(loaderControlURL, optarg);
					break;
				case 4: // "vuo-runner"
					runnerPid = atoi(optarg);
					break;
			}
		}
	}

	// Set up ZMQ connections.
	{
		ZMQLoaderControlContext = zmq_init(1);

		ZMQLoaderControl = zmq_socket(ZMQLoaderControlContext,ZMQ_REP);
		if(zmq_bind(ZMQLoaderControl,loaderControlURL))
		{
			VLog("Error: Bind '%s' failed.", loaderControlURL);
			free(loaderControlURL);
			return 1;
		}
		free(loaderControlURL);
	}

	// Launch control responder.
	dispatch_queue_t loaderControlQueue;
	dispatch_source_t loaderControlTimer;
	dispatch_semaphore_t loaderControlCanceledSemaphore;
	{
		loaderControlCanceledSemaphore = dispatch_semaphore_create(0);
		loaderControlQueue = dispatch_queue_create("org.vuo.runtime.loader", NULL);
		loaderControlTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,loaderControlQueue);
		dispatch_source_set_timer(loaderControlTimer, dispatch_walltime(NULL,0), NSEC_PER_SEC/1000, NSEC_PER_SEC/1000);
		dispatch_source_set_event_handler(loaderControlTimer, ^{

											  vuoMemoryBarrier();

											  zmq_pollitem_t items[]=
											  {
												  {ZMQLoaderControl,0,ZMQ_POLLIN,0},
											  };
											  int itemCount = 1;
											  long timeout = 1000;  // wait interruptably, to be able to cancel this dispatch source when the composition stops
											  zmq_poll(items,itemCount,timeout);
											  if(!(items[0].revents & ZMQ_POLLIN))
											  return;

											  enum VuoLoaderControlRequest control = (enum VuoLoaderControlRequest) vuoReceiveInt(ZMQLoaderControl);

											  switch (control)
											  {
												  case VuoLoaderControlRequestCompositionReplace:
												  {
													  char *dylibPath = vuoReceiveAndCopyString(ZMQLoaderControl);
													  char *resourceDylibPath = vuoReceiveAndCopyString(ZMQLoaderControl);
													  char *compositionDiff = vuoReceiveAndCopyString(ZMQLoaderControl);
													  loadResourceDylib(resourceDylibPath);
													  replaceComposition(dylibPath, compositionDiff);
													  free(dylibPath);
													  free(resourceDylibPath);
													  free(compositionDiff);

													  vuoLoaderControlReplySend(VuoLoaderControlReplyCompositionReplaced,NULL,0);
													  break;
												  }
											  }
										  });
		dispatch_source_set_cancel_handler(loaderControlTimer, ^{
											   dispatch_semaphore_signal(loaderControlCanceledSemaphore);
										   });
		dispatch_resume(loaderControlTimer);
	}

	// Wait until the composition is permanently stopped (not just temporarily stopped for replacing).
	{
		bool isStoppedInitially = false;
		isStopped = &isStoppedInitially;
		while (isReplacing || ! *isStopped)  // Check isReplacing first, since isStopped is invalid for part of the time that isReplacing is true.
		{
			id pool = objc_msgSend((id)objc_getClass("NSAutoreleasePool"), sel_getUid("new"));
			CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.01,false);
			objc_msgSend(pool, sel_getUid("drain"));
		}
	}

	// Clean up ZMQ connections.
	{
		vuoMemoryBarrier();

		zmq_close(ZMQControl);

		dispatch_source_cancel(loaderControlTimer);
		dispatch_semaphore_wait(loaderControlCanceledSemaphore, DISPATCH_TIME_FOREVER);
		dispatch_release(loaderControlCanceledSemaphore);
		dispatch_release(loaderControlTimer);
		dispatch_sync(loaderControlQueue, ^{
			zmq_close(ZMQLoaderControl);
		});
		dispatch_release(loaderControlQueue);

		typedef void(* vuoFiniType)(void);
		vuoFiniType vuoFini = (vuoFiniType) dlsym(dylibHandle, "vuoFini");
		if (! vuoFini)
		{
			VLog("Error: Couldn't find function 'vuoFini': %s", dlerror());
			return 1;
		}
		vuoFini();
	}

	unloadResourceDylibs();

	return 0;
}

/**
 * Replaces the currently running composition (if any) with the given composition.
 */
void replaceComposition(const char *dylibPath, const char *updatedCompositionDiff)
{
	isReplacing = true;

	bool isPausedOnStart = *isPaused;  // Store isPaused, since it will become invalid when the old composition is unloaded.
	bool isStopRequestedByComposition = false;

	// Serialize and stop the old composition (if any).
	char *serializedComposition = NULL;
	if (dylibHandle)
	{
		const char **compositionDiff = (const char**)dlsym(dylibHandle, "compositionDiff");
		if (! compositionDiff)
		{
			VLog("Error: Couldn't find variable 'compositionDiff': %s", dlerror());
			return;
		}
		*compositionDiff = updatedCompositionDiff;

		if (! *isPaused)
			pauseComposition();

		typedef char * (* vuoSerializeType)(void);
		vuoSerializeType vuoSerialize = (vuoSerializeType) dlsym(dylibHandle, "vuoSerialize");
		if (! vuoSerialize)
		{
			VLog("Error: Couldn't find function 'vuoSerialize': %s", dlerror());
			return;
		}

		serializedComposition = vuoSerialize();

		stopComposition();

		bool *isStopRequested = (bool *)dlsym(dylibHandle, "isStopRequested");
		if (! isStopRequested)
		{
			VLog("Error: Couldn't find variable 'isStopRequested': %s", dlerror());
			return;
		}
		isStopRequestedByComposition = *isStopRequested;

		zmq_close(ZMQControl);
		ZMQControl = NULL;

		typedef void(* vuoFiniType)(void);

		vuoFiniType vuoFini = (vuoFiniType)dlsym(dylibHandle, "vuoFini");
		if (! vuoFini)
		{
			VLog("Error: Couldn't find function 'vuoFini': %s", dlerror());
			return;
		}
		vuoFini();

		isStopped = NULL;
		isPaused = NULL;

		dlclose(dylibHandle);
		dylibHandle = NULL;
	}

	// Start the new composition paused.
	{
		ZMQControlContext = zmq_init(1);

		ZMQControl = zmq_socket(ZMQControlContext,ZMQ_REQ);
		if (zmq_connect(ZMQControl,controlURL))
		{
			VLog("Error: Connect to '%s' failed.", controlURL);
			return;
		}

		typedef void(* vuoInitInProcessType)(void *_ZMQContext, const char *controlURL, const char *telemetryURL, bool _isPaused, pid_t _runnerPid);
		vuoInitInProcessType vuoInitInProcess = NULL;

		dylibHandle = dlopen(dylibPath, RTLD_NOW);
		if (! dylibHandle)
		{
			VLog("Error: Couldn't load dylib: %s", dlerror());
			return;
		}

		vuoInitInProcess = (vuoInitInProcessType)dlsym(dylibHandle, "vuoInitInProcess");
		if (! vuoInitInProcess)
		{
			VLog("Error: Couldn't find function 'vuoInitInProcess': %s", dlerror());
			return;
		}

		isStopped = (bool *)dlsym(dylibHandle, "isStopped");
		if (! isStopped)
		{
			VLog("Error: Couldn't find variable 'isStopped': %s", dlerror());
			return;
		}

		isPaused = (bool *)dlsym(dylibHandle, "isPaused");
		if (! isPaused)
		{
			VLog("Error: Couldn't find variable 'isPaused': %s", dlerror());
			return;
		}

		vuoInitInProcess(ZMQControlContext, controlURL, telemetryURL, true, runnerPid);
	}

	// Unserialize the old composition's state (if any) into the new composition.
	const char **compositionDiff = NULL;
	if (serializedComposition)
	{
		compositionDiff = (const char **)dlsym(dylibHandle, "compositionDiff");
		if (! compositionDiff)
		{
			VLog("Error: Couldn't find variable 'compositionDiff': %s", dlerror());
			return;
		}
		*compositionDiff = updatedCompositionDiff;

		typedef void (* vuoUnserializeType)(char *);
		vuoUnserializeType vuoUnserialize = (vuoUnserializeType) dlsym(dylibHandle, "vuoUnserialize");

		vuoUnserialize(serializedComposition);
	}

	// Unpause the new composition (if needed).
	if (! isPausedOnStart)
	{
		unpauseComposition();
	}

	// Reset the composition's `compositionDiff` back to NULL, since this function's caller is about to free its pointee.
	if (serializedComposition)
	{
		*compositionDiff = NULL;
	}

	// If the composition had a pending call to vuoStopComposition() when it was stopped, call it again.
	if (isStopRequestedByComposition)
	{
		typedef void (*vuoStopCompositionType)(void);
		vuoStopCompositionType vuoStopComposition = (vuoStopCompositionType) dlsym(dylibHandle, "vuoStopComposition");
		if (! vuoStopComposition)
		{
			VLog("Error: Couldn't find function 'vuoStopComposition': %s", dlerror());
			return;
		}
		vuoStopComposition();
	}

	isReplacing = false;
}

/**
 * Sends a control request to the composition telling it to stop.
 */
void stopComposition(void)
{
	vuoMemoryBarrier();

	const int timeoutInSeconds = -1;
	zmq_msg_t messages[1];
	vuoInitMessageWithInt(&messages[0], timeoutInSeconds);
	vuoControlRequestSend(VuoControlRequestCompositionStop,messages,1);
	vuoControlReplyReceive(VuoControlReplyCompositionStopping);
}

/**
 * Sends a control request to the composition telling it to pause.
 */
void pauseComposition(void)
{
	vuoMemoryBarrier();

	vuoControlRequestSend(VuoControlRequestCompositionPause,NULL,0);
	vuoControlReplyReceive(VuoControlReplyCompositionPaused);
}

/**
 * Sends a control request to the composition telling it to unpause.
 */
void unpauseComposition(void)
{
	vuoMemoryBarrier();

	vuoControlRequestSend(VuoControlRequestCompositionUnpause,NULL,0);
	vuoControlReplyReceive(VuoControlReplyCompositionUnpaused);
}

/**
 * Loads the resource dylib at the given path and adds the resulting handle to the list of resource dylibs.
 *
 * If @c resourceDylibPath is the empty string, does nothing.
 */
void loadResourceDylib(const char *resourceDylibPath)
{
	if (strlen(resourceDylibPath) == 0)
		return;

	void *resourceDylibHandle = dlopen(resourceDylibPath, RTLD_NOW);
	if (! resourceDylibHandle)
	{
		VLog("Error: Couldn't load resource dylib: %s", dlerror());
		return;
	}

	if (resourceDylibHandlesSize == 0)
	{
		resourceDylibHandlesCapacity = 1;
		resourceDylibHandles = (void **)malloc(resourceDylibHandlesCapacity * sizeof(void *));
	}
	else if (resourceDylibHandlesSize == resourceDylibHandlesCapacity)
	{
		resourceDylibHandlesCapacity *= 2;
		void **newResourceDylibHandles = (void **)malloc(resourceDylibHandlesCapacity * sizeof(void *));
		for (int i = 0; i < resourceDylibHandlesSize; ++i)
			newResourceDylibHandles[i] = resourceDylibHandles[i];
		free(resourceDylibHandles);
		resourceDylibHandles = newResourceDylibHandles;
	}

	resourceDylibHandles[resourceDylibHandlesSize++] = resourceDylibHandle;

	if (! isReferenceCountingInitialized)
	{
		typedef void (*initType)(void);
		initType VuoHeap_init = (initType) dlsym(resourceDylibHandle, "VuoHeap_init");
		if (! VuoHeap_init)
		{
			VLog("Error: Couldn't find function 'VuoHeap_init': %s", dlerror());
			return;
		}
		VuoHeap_init();

		isReferenceCountingInitialized = true;
	}
}

/**
 * Unloads all resource dylibs.
 */
void unloadResourceDylibs(void)
{
	for (int i = 0; i < resourceDylibHandlesSize; ++i)
		dlclose(resourceDylibHandles[i]);
}
