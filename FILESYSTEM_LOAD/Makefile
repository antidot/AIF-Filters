#***************************************************************************
# (C) 2012 Antidot
#
# Makefile du module	:	Filesystem load filter
#
#***************************************************************************
MODULES			=	

LIB			=	AFS_FILESYSTEM_LOAD

LIB_OBJECTS		=	fs_load.o fs_proxy.o fs_mount.o fs_samba.o fs_url.o

EXE			=	afs_filesystem_load

EXE_OBJECTS		=	main.o

USE_LIBS		=	$(AFS_PaF_API_RTL) -lAFS_SECURITY -lsmbclient \
				$(AFS_PaF) $(CONF_LINK) $(COMMON_LINK) $(SYS_LINK)

include $(DEV_ROOT)/src/makerules/antidot.mk

#
# Fin du fichier
#
