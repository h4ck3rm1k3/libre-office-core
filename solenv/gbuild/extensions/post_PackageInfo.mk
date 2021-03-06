# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

gb_PackageInfo_InstallModules := \
	base \
	calc \
	graphicsfilter \
	tde \
	impress \
	onlineupdate \
	gnome \
	kde \
	math \
	ooo \
	writer \
	ure \
	activexbinarytable \
	ooobinarytable \
	winexplorerextbinarytable \

define gb_PackageInfo_emit_binaries_command
@touch $(foreach suf,executables libraries files,$(gb_PackageInfo_get_target)/$(1).$(suf))
@$(foreach executable,$(gb_Executable_MODULE_$(1)),echo "$(patsubst $(INSTDIR)/%,%,$(call gb_Executable_get_target,$(executable)))" >> $(gb_PackageInfo_get_target)/$(1).executables &&) true
@$(foreach library,$(gb_Library_MODULE_$(1)),echo "$(patsubst $(INSTDIR)/%,%,$(call gb_Library_get_target,$(library)))" >> $(gb_PackageInfo_get_target)/$(1).libraries &&) true

endef

define gb_PackageInfo_emit_help_for_one_target
$(foreach suf,cfg db ht idxl/_0.cfs idxl/segments_3 idxl/segments.gen jar key tree,$(if $(wildcard $(INSTDIR)/help/$(1).$(suf)),echo "help/$(1).$(suf)" >> $(2) && )) true

endef

define gb_PackageInfo_emit_help_for_one_lang
@touch $(foreach suf,executables libraries files,$(gb_PackageInfo_get_target)/help-$(1).$(suf))
$(foreach target,$(gb_AllLangHelp_ALLTARGETS),$(call gb_PackageInfo_emit_help_for_one_target,$(1)/$(target),$(gb_PackageInfo_get_target)/help-$(1).files))
$(foreach suf,html css,$(foreach file,$(wildcard $(INSTDIR)/help/$(1)/*.$(suf)),echo "$(patsubst $(INSTDIR)/%,%,$(file))" >> $(gb_PackageInfo_get_target)/help-$(1).files && )) true

endef

#getting the package files post-hoc with wildcard isnt good and should be done better
define gb_PackageInfo_emit_l10n_for_one_alllangpackage
@$(foreach file,$(shell ls $(INSTDIR)/$(1)/$(2)),echo "$(1)/$(2)/$(file)" >> $(gb_PackageInfo_get_target)/l10n-$(2).files &&) true

endef

define gb_PackageInfo_emit_l10n_for_one_ressource
@echo "$(patsubst $(INSTDIR)/%,%,$(call gb_ResTarget_get_install_target,$(1)$(2)))" >> $(gb_PackageInfo_get_target)/l10n-$(2).files

endef

define gb_PackageInfo_emit_l10n_for_one_uifile
@echo "$(gb_UIConfig_INSTDIR)/$(2)/ui/res/$(1)/$(3)" >> $(gb_PackageInfo_get_target)/l10n-$(1).files

endef

define gb_PackageInfo_emit_l10n_for_one_configfile
echo "$(LIBO_SHARE_FOLDER)/registry/$(2)$(1).xcd" >> $(gb_PackageInfo_get_target)/l10n-$(1).files

endef 

define gb_PackageInfo_emit_l10n_for_one_lang
@touch $(foreach suf,executables libraries files,$(gb_PackageInfo_get_target)/l10n-$(1).$(suf))
$(if $(filter-out qtz en-US,$(1)),$(foreach packagedir,$(patsubst %/,%,$(gb_AllLangPackage_ALLDIRS)),$(call gb_PackageInfo_emit_l10n_for_one_alllangpackage,$(packagedir),$(1))))
$(if $(filter $(gb_AllLangResTarget_LANGS),$(1)),$(foreach target,$(gb_AllLangResTarget_ALLTARGETS),$(call gb_PackageInfo_emit_l10n_for_one_ressource,$(target),$(1))))
$(foreach uifile,$(gb_UIConfig_ALLFILES),$(call gb_PackageInfo_emit_l10n_for_one_uifile,$(1),$(firstword $(subst :,$(WHITESPACE),$(uifile))),$(lastword $(subst :,$(WHITESPACE),$(uifile)))))
$(if $(filter $(gb_Configuration_LANGS),$(1)),$(foreach configfile,Langpack- res/fcfg_langpack_ res/registry_,$(call gb_PackageInfo_emit_l10n_for_one_configfile,$(1),$(configfile))))

endef

.PHONY: packageinfo
$(foreach filelist,files executables libraries,$(gb_PackageInfo_get_target)/%.$(filelist)):
	@rm -rf $(gb_PackageInfo_get_target) && mkdir $(gb_PackageInfo_get_target)
	$(foreach installmodule,$(gb_PackageInfo_InstallModules),$(call gb_PackageInfo_emit_binaries_command,$(installmodule)))
	$(foreach helplang,$(gb_HELP_LANGS),$(call gb_PackageInfo_emit_help_for_one_lang,$(helplang)))
	$(foreach l10nlang,$(if $(strip $(gb_WITH_LANG)),$(gb_WITH_LANG),en-US),$(call gb_PackageInfo_emit_l10n_for_one_lang,$(l10nlang)))

packageinfo: $(gb_PackageInfo_get_target)/ure.files

install-package-%: $(foreach filelist,files executables libraries,$(gb_PackageInfo_get_target)/%.$(filelist))
	for executable in `cat $(gb_PackageInfo_get_target)/$*.executables`; \
	do \
		install -D $(INSTDIR)/$${executable} $(INSTALLDIR)/$${executable} ;\
	done
	for library in `cat $(gb_PackageInfo_get_target)/$*.libraries`; \
	do \
		install -D -m644 $(INSTDIR)/$${library} $(INSTALLDIR)/$${library}; \
	done
	for file in `cat $(gb_PackageInfo_get_target)/$*.files`; \
	do \
		install -D -m644 $(INSTDIR)/$${file} $(INSTALLDIR)/$${file}; \
	done

# vim: set noet sw=4 ts=4:
