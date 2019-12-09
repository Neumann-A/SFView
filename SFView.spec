%define package_version 0

Summary	  : MPI System function analysis tool
Name	  : SFView
Version	  : 1.0
Release	  : %{package_version}
Group	  : Application/Science
Source	  : SFView.tar.gz
License	  : GPL
BuildRoot : $RPM_BUILD_ROOT

%description
This package provides a tool for analysis of MPI system matrices acquired with the Bruker Preclinical MPI System.

%package edit
Summary   : MPI System function editing tool
Group     : Application/Science
%description edit
This package provides a tool for editing of MPI system matrices acquired with the Bruker Preclinical MPI System.

%prep
%setup -n SFView

%build
qmake RELEASE=${VERSION}%{package_version}
make

%install
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/applications
mkdir -p $RPM_BUILD_ROOT/usr/share/mime/packages
mkdir -p $RPM_BUILD_ROOT/%{_defaultdocdir}/SFView
for D in 16x16 22x22 32x32 48x48 64x64 96x96 128x128; do
    mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/$D/apps
    mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/$D/mimetypes
    cp SFView-$D.png $RPM_BUILD_ROOT/usr/share/icons/hicolor/$D/apps/sfview.png
    cp SFView-$D.png $RPM_BUILD_ROOT/usr/share/icons/hicolor/$D/mimetypes/mpi-systemmatrix.png
    cp SFEdit-$D.png $RPM_BUILD_ROOT/usr/share/icons/hicolor/$D/apps/sfedit.png
done
cp application-x-bruker-mpi-systemmatrix.xml $RPM_BUILD_ROOT/usr/share/mime/packages
cp SFView $RPM_BUILD_ROOT/usr/bin
pushd $RPM_BUILD_ROOT/usr/bin
ln -s SFView SFEdit
popd
cp SFView.desktop SFEdit.desktop $RPM_BUILD_ROOT/usr/share/applications
cp ChangeLog README TODO AUTHORS $RPM_BUILD_ROOT/%{_defaultdocdir}/SFView

%clean
rm -rf $RPM_BUILD_ROOT

%post
update-mime-database > /dev/null /usr/share/mime

%postun
update-mime-database > /dev/null /usr/share/mime

%files
%defattr (-,root,root)
/usr/bin/SFView
/usr/share/applications/SFView.desktop
/usr/share/icons/hicolor/16x16/apps/sfview.png
/usr/share/icons/hicolor/22x22/apps/sfview.png
/usr/share/icons/hicolor/32x32/apps/sfview.png
/usr/share/icons/hicolor/48x48/apps/sfview.png
/usr/share/icons/hicolor/64x64/apps/sfview.png
/usr/share/icons/hicolor/96x96/apps/sfview.png
/usr/share/icons/hicolor/128x128/apps/sfview.png
/usr/share/icons/hicolor/16x16/mimetypes/mpi-systemmatrix.png
/usr/share/icons/hicolor/22x22/mimetypes/mpi-systemmatrix.png
/usr/share/icons/hicolor/32x32/mimetypes/mpi-systemmatrix.png
/usr/share/icons/hicolor/48x48/mimetypes/mpi-systemmatrix.png
/usr/share/icons/hicolor/64x64/mimetypes/mpi-systemmatrix.png
/usr/share/icons/hicolor/96x96/mimetypes/mpi-systemmatrix.png
/usr/share/icons/hicolor/128x128/mimetypes/mpi-systemmatrix.png
/usr/share/mime/packages/application-x-bruker-mpi-systemmatrix.xml
%doc /%{_defaultdocdir}/SFView/ChangeLog
%doc /%{_defaultdocdir}/SFView/README
%doc /%{_defaultdocdir}/SFView/TODO
%doc /%{_defaultdocdir}/SFView/AUTHORS

%files edit
/usr/bin/SFEdit
/usr/share/applications/SFEdit.desktop
/usr/share/icons/hicolor/16x16/apps/sfedit.png
/usr/share/icons/hicolor/22x22/apps/sfedit.png
/usr/share/icons/hicolor/32x32/apps/sfedit.png
/usr/share/icons/hicolor/48x48/apps/sfedit.png
/usr/share/icons/hicolor/64x64/apps/sfedit.png
/usr/share/icons/hicolor/96x96/apps/sfedit.png
/usr/share/icons/hicolor/128x128/apps/sfedit.png

%changelog
* Sun Mar 19 2017 Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
- Preparations for moving support for Bruker system functions into subclass
- Official 1.0 release for IWMPI 2017
* Tue Feb 28 2017 Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
- Port experimental spectral plot to QtCharts and promote to official feature
- Place Phase view in separate tool
* Fri Jan 27 2017 Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
- Bug fixes in coefficient navigation (no update of harmonic and mixing depth)
- Fix critical integer overflows in SystemMatrix class which occured for large datasets
* Wed Jan 25 2017 Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
- Improved phase view
* Sun Dec 11 2016 Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
- Toolbars with scalable icons added
- Color scales are now handled in separate infrastructure
* Wed Nov 09 2016 Ulrich Heinen <ulrich.heinen@hs-pforzheim.de>
- Color scale starts at value 0 by default now (configurable)
- Show phase labels in phase plot
- Show variance in phase plot (configurable)
- Fix issue where navigation spinboxes were no longer updated.
* Fri Sep 23 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Turn phase viewer into "official" feature
* Thu Sep 22 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Experimental phase viewer now has a highlighting cross-link to main view and vice versa
* Fri Mar 11 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Register mimetype and couple application to it.
* Wed Mar 09 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Support files/paths passed on the command line
* Tue Mar 01 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Minor layout corrections
- Internal preparations for a spectrum display
* Thu Feb 25 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Fixed packaging of viewer and editor
- Ensure that variant (Viewer/Editor) is shown correctly in title and about dialog
- Include release number in displayed version number
- Minor correction in mixing terms update and plot layout
* Mon Feb 15 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Protect critical operations by additional message dialogs.
* Fri Feb 12 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Adjust color legend for single slice view
- Add display of information about acquisition system
- Add Undo function in edit mode
* Mon Feb 08 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Reworked modification table to cover corrected and uncorrected data.
- Split packages into Viewer and Editor
* Thu Jan 28 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Map additional files into memory to speed up loading
- Load full background file only in case of edit mode
- Defer background noise calculation until needed
- Prepare for correction of both correction and uncorrected representation
* Thu Jan 14 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Restrict voxel correction to background corrected view.
- Add versioning to modification table
* Tue Jan 12 2016 Ulrich Heinen <ulrich.heinen@arcor.de>
- Added icons and separate invocation option as editor.
* Mon Dec 14 2015 Ulrich Heinen <ulrich.heinen@arcor.de>
- Initial package preparation
