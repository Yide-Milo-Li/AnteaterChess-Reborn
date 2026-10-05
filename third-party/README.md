# Third-party notices

The application dynamically links Qt. Qt libraries remain replaceable with
compatible libraries; users retain the rights granted by their Qt licenses,
including reverse engineering for debugging modifications to those libraries.
The application's original COPYRIGHT remains unchanged.

Windows uses the official Qt 6.11.2 MSVC 2022 x64 kit. The runtime receipt includes
Qt's vendor SBOM, component copyrights, source repositories, license identifiers
and extracted license texts. This directory preserves the official source-tag
license texts and Qt documentation attribution pages. provenance.json records
their sources and SHA-256 values. Some notices cover optional Qt components;
their presence does not mean that all such components are deployed.

Corresponding Qt sources: https://download.qt.io/official_releases/qt/6.11/6.11.2/
and the Qt repositories at https://code.qt.io/cgit/qt/ (tag v6.11.2).
Qt licensing: https://doc.qt.io/qt-6.11/licensing.html.

Microsoft runtime DLLs originate exclusively from the installed v143 14.44 x64
Release REDIST directory. Their redistribution is subject to the Visual Studio
license terms and the Microsoft distributable-code list retained here. Debug CRT
files are never included. License directory:
https://visualstudio.microsoft.com/license-terms/.

Linux packages do not bundle Qt or system ELF libraries. DEPENDENCIES.json records
the installed Ubuntu package versions, QML modules, platform/image plugins and
ELF dependency closure. Their package copyright and common license texts are
included under licenses/system. Install dependencies from Ubuntu 24.04's package
repositories; a package version inventory is evidence for that measured host.
