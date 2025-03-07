/**
 * @file
 * VuoTelemetry implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VuoTelemetry.h"

/**
 * Copies the message data into a newly allocated string.
 *
 * Assumes the message data includes a null terminator.
 *
 * If the message is zero-length, returns NULL.
 */
char * vuoCopyStringFromMessage(zmq_msg_t *message)
{
	size_t messageSize = zmq_msg_size(message);
	if (!messageSize)
		return NULL;
	char *string = (char *)malloc(messageSize);
	memcpy(string, zmq_msg_data(message), messageSize);
	return string;
}

/**
 * Copies the string (including null terminator) into the message data.
 */
void vuoInitMessageWithString(zmq_msg_t *message, const char *string)
{
	size_t messageSize = (string != NULL ? (strlen(string) + 1) : 0);
	zmq_msg_init_size(message, messageSize);
	memcpy(zmq_msg_data(message), string, messageSize);
}

/**
 * Copies the int value into the message data.
 */
void vuoInitMessageWithInt(zmq_msg_t *message, int value)
{
	size_t messageSize = sizeof(int);
	zmq_msg_init_size(message, messageSize);
	memcpy(zmq_msg_data(message), &value, messageSize);
}

/**
 * Copies the bool value into the message data.
 */
void vuoInitMessageWithBool(zmq_msg_t *message, bool value)
{
	size_t messageSize = sizeof(bool);
	zmq_msg_init_size(message, messageSize);
	memcpy(zmq_msg_data(message), &value, messageSize);
}

/**
 * Receives the next message on the socket and copies it into a newly allocated string.
 */
char * vuoReceiveAndCopyString(void *socket)
{
	zmq_msg_t message;
	zmq_msg_init(&message);

	if (zmq_recv(socket, &message, 0) != 0)
	{
		VLog("Error: Connection timed out: %s", strerror(errno));
		return NULL;
	}

	char *string = vuoCopyStringFromMessage(&message);
	zmq_msg_close(&message);
	return string;
}

/**
 * Helper for @c vuoReceiveInt() and @c vuoReceiveUnsignedLong().
 */
void vuoReceiveBlocking(void *socket, void *data, size_t dataSize)
{
	zmq_msg_t message;
	zmq_msg_init(&message);

	if (zmq_recv(socket, &message, 0) != 0)
	{
		VLog("Error: Connection timed out: %s", strerror(errno));
		bzero(data, dataSize);
		return;
	}

	size_t messageSize = zmq_msg_size(&message);
	if (messageSize != dataSize)
	{
		VLog("Error: vuoReceiveBlocking() expected %lu bytes of data, but actually received %lu.", (unsigned long)dataSize, (unsigned long)messageSize);
		bzero(data, dataSize);
		return;
	}
	memcpy(data, zmq_msg_data(&message), messageSize);
	zmq_msg_close(&message);
}

/**
 * Receives the next message on the socket and copies it into an unsigned long.
 */
unsigned long vuoReceiveUnsignedInt64(void *socket)
{
	uint64_t number = 0;
	vuoReceiveBlocking(socket, (void *)&number, sizeof(number));
	return number;
}

/**
 * Receives the next message on the socket and copies it into an int.
 */
int vuoReceiveInt(void *socket)
{
	int number = 0;
	vuoReceiveBlocking(socket, (void *)&number, sizeof(number));
	return number;
}

/**
 * Receives the next message on the socket and copies it into a bool.
 */
bool vuoReceiveBool(void *socket)
{
	bool value = false;
	vuoReceiveBlocking(socket, (void *)&value, sizeof(value));
	return value;
}

/**
 * Sends the multipart message @c messages on ZMQ socket @c socket.
 * @c name is just used for printing error messages.
 */
void vuoSend(const char *name, void *socket, int type, zmq_msg_t *messages, unsigned int messageCount, bool isNonBlocking)
{
	// send the type message-part
	{
		zmq_msg_t message;
		zmq_msg_init_size(&message, sizeof type);
		memcpy(zmq_msg_data(&message), &type, sizeof type);
		int flags = (messageCount>0 ? ZMQ_SNDMORE : 0) | (isNonBlocking ? ZMQ_NOBLOCK : 0);
		if(zmq_send(socket, &message, flags))
			VLog("Error: Failed to send type for '%s'.", name);
		zmq_msg_close(&message);
	}

	// send the data message-parts
	for(unsigned int i=0; i<messageCount; ++i)
	{
		int flags = (i<messageCount-1 ? ZMQ_SNDMORE : 0) | (isNonBlocking ? ZMQ_NOBLOCK : 0);
		if(zmq_send(socket, &messages[i], flags))
			VLog("Error: Failed to send data %u for '%s'.", i, name);
		zmq_msg_close(&messages[i]);
	}
}

#include <libkern/OSAtomic.h>
/**
 * "Individual ØMQ sockets are not thread safe except in the case where full memory barriers
 * are issued when migrating a socket from one thread to another."
 *
 * http://api.zeromq.org/2-2:zmq
 * http://stackoverflow.com/questions/5841896/0mq-how-to-use-zeromq-in-a-threadsafe-manner
 * https://b33p.net/kosada/node/4226
 */
void vuoMemoryBarrier(void)
{
	OSMemoryBarrier();
}
