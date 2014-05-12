# Based on the Gentoo ebuild:
# Copyright 1999-2012 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

EAPI="4"

inherit autotools cros-workon

RESTRICT="nomirror"
DESCRIPTION="Khronos openMAX IL Core library"
HOMEPAGE="https://github.com/01org/omxil_core"

if [[ "${PV}" != "9999" ]] ; then
SRC_URI="https://github.com/01org/omxil_core/archive/${P}.tar.gz"
else
CROS_WORKON_LOCALNAME="../partner_private/omxil-core"
fi

LICENSE="Apache-2.0"
SLOT="0"
KEYWORDS="~-* ~amd64 ~x86"
IUSE="omx-debug"

RDEPEND="x11-libs/libva"

DEPEND="${RDEPEND}
	virtual/pkgconfig"

if [[ "${PV}" != "9999" ]] ; then
src_unpack() {
	default
	# tag and project names are fixed in github
	S="${WORKDIR}/omxil_core-${P}"
}
fi

src_prepare() {
	eautoreconf
}

src_configure() {
	econf $(use_enable omx-debug)
}

src_install() {
	default
	prune_libtool_files
}
