TEMPLATE = aux
CONFIG += VuoNodeSet

include(../../vuo.pri)

NODE_SOURCES += \
	vuo.quaternion.make.angle.c \
	vuo.quaternion.make.vectors.c

include(../../module.pri)
