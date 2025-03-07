TEMPLATE = lib
CONFIG += staticlib json
TARGET = VuoTypeList

include(../../vuo.pri)

OTHER_FILES += \
	VuoList.cc \
	VuoList.h


# Generate the list type variants, and build the generated list type variants.
# The number after the colon means:
#    0 — type is not reference counted
#    1 — type is reference counted, using VuoRetain() and VuoRelease()
#    2 — type is reference counted, using ELEMENT_TYPE_retain() and ELEMENT_TYPE_release()
TYPE_LIST_SOURCES = $$system( ./generateVariants.sh \
		VuoArtNetInputDevice:2 \
		VuoArtNetOutputDevice:2 \
		VuoAudioInputDevice:2 \
		VuoAudioOutputDevice:2 \
		VuoAudioSamples:2 \
		VuoBlendMode:0 \
		VuoBoolean:0 \
		VuoColor:0 \
		VuoCursor:2 \
		VuoCurve:0 \
		VuoCurveEasing:0 \
		VuoDispersion:0 \
		VuoDisplacement:0 \
		VuoDmxColorMap:2 \
		VuoFileType:0 \
		VuoFont:2 \
		VuoGradientNoise:0 \
		VuoHorizontalAlignment:0 \
		VuoHorizontalReflection:0 \
		VuoHorizontalSide:0 \
		VuoIconPosition:2 \
		VuoImage:1 \
		VuoImageColorDepth:0 \
		VuoImageWrapMode:0 \
		VuoInteger:0 \
		VuoKey:0 \
		VuoLayer:2 \
		VuoLeapFrame:2 \
		VuoLeapHand:2 \
		VuoLeapPointable:0 \
		VuoLeapPointableType:0 \
		VuoLeapTouchZone:0 \
		VuoListPosition:2 \
		VuoLoopType:0 \
		VuoMesh:2 \
		VuoMidiController:0 \
		VuoMidiInputDevice:2 \
		VuoMidiOutputDevice:2 \
		VuoMidiNote:0 \
		VuoMidiPitchBend:2 \
		VuoModifierKey:0 \
		VuoMouseButton:0 \
		VuoNoise:0 \
		VuoNotePriority:2 \
		VuoOscMessage:2 \
		VuoPoint2d:0 \
		VuoPoint3d:0 \
		VuoPoint4d:0 \
		VuoReal:0 \
		VuoRealRegulation:2 \
		VuoRenderedLayers:2 \
		VuoRssItem:2 \
		VuoSceneObject:2 \
		VuoScreen:2 \
		VuoShader:1 \
		VuoImageFormat:0 \
		VuoSizingMode:0 \
		VuoSyphonServerDescription:0 \
		VuoText:1 \
		VuoThresholdType:0 \
		VuoTransform2d:0 \
		VuoTransform:0 \
		VuoUrl:2 \
		VuoVertexAttribute:2 \
		VuoVerticalAlignment:0 \
		VuoVerticalReflection:0 \
		VuoVideoFrame:2 \
		VuoVideoInputDevice:2 \
		VuoWave:0 \
		VuoWindowProperty:2 \
		VuoWindowReference:0 \
		VuoWrapMode:0 \
	)

TYPE_INCLUDEPATH = \
	$$ROOT/node/vuo.artnet \
	$$ROOT/node/vuo.audio \
	$$ROOT/node/vuo.bcf2000 \
	$$ROOT/node/vuo.color \
	$$ROOT/node/vuo.file \
	$$ROOT/node/vuo.font \
	$$ROOT/node/vuo.image \
	$$ROOT/node/vuo.keyboard \
	$$ROOT/node/vuo.layer \
	$$ROOT/node/vuo.leap \
	$$ROOT/node/vuo.list \
	$$ROOT/node/vuo.midi \
	$$ROOT/node/vuo.mouse \
	$$ROOT/node/vuo.motion \
	$$ROOT/node/vuo.video \
	$$ROOT/node/vuo.noise \
	$$ROOT/node/vuo.osc \
	$$ROOT/node/vuo.rss \
	$$ROOT/node/vuo.scene \
	$$ROOT/node/vuo.shader \
	$$ROOT/node/vuo.syphon \
	$$ROOT/node/vuo.ui

include(../../module.pri)

typeList.input = TYPE_LIST_SOURCES
typeList.output = ${QMAKE_FILE_IN_BASE}.bc
typeList.depends = $$OTHER_FILES ../*.h
typeList.commands  = $$VUOCOMPILE $$VUOCOMPILE_TYPE_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeList.variable_out = TYPE_LIST_OBJECTS
typeList.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeList

typeListObjects.input = TYPE_LIST_OBJECTS
typeListObjects.output = ${QMAKE_FILE_IN_BASE}.o
typeListObjects.commands = $$QMAKE_CC -Oz -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
typeListObjects.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += typeListObjects

OTHER_FILES += $$TYPE_LIST_SOURCES

HEADERS = VuoList_*.h

INCLUDEPATH += \
	$$ROOT/node \
	$$ROOT/type \
	$$ROOT/runtime

QMAKE_CLEAN += VuoList_*.cc VuoList_*.h
