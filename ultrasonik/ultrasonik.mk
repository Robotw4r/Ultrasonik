ULTRASONIK_VERSION = 1.0
ULTRASONIK_SITE = $(TOPDIR)/package/ultrasonik/src
ULTRASONIK_SITE_METHOD = local

$(eval $(kernel-module))
$(eval $(generic-package))
