# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,mingw_gccdlls,$(MINGW_SYSROOT)/bin))

$(error FIXME do not deliver this stuff to OUTDIR)

$(eval $(call gb_Package_add_files,mingw_gccdlls,bin,\
    $(if $(filter YES,$(MINGW_SHARED_GCCLIB)),$(MINGW_GCCDLL)) \
    $(if $(filter YES,$(MINGW_SHARED_GXXLIB)),$(MINGW_GXXDLL)) \
))

# vim:set shiftwidth=4 tabstop=4 noexpandtab:
