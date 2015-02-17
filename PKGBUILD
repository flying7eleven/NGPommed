# Maintainer: Tim Hütz <tim@huetz.biz>

# Forked from pommed. Previous header:
# Maintainer: Corrado Primier <ilbardo gmail com>
# Contributor: Other contributors existed but lost information
# Contributor: dpevp <daniel.plaza.espi@gmail.com>
# Contributor: jordi Cerdan (jcerdan) <jcerdan@tecob.com>

pkgname=pommed-ng
pkgver=1.0.1
pkgrel=1
pkgdesc="Handles the hotkeys of Apple MacBook (Pro) laptops"
arch=('i686' 'x86_64')
url="https://github.com/thuetz/NGPommed"
license=('GPL2')
makedepends=('pkgconfig')
depends=('confuse' 'audiofile' 'pciutils' 'dbus-glib') # alsa-lib reference is fullfilled by audiofile
conflicts=('pommed','pommed-light', 'pommed-git', 'pommed-jalaziz', 'gpomme')
optdepends=('eject: disc ejection support')
source=( "https://github.com/thuetz/NGPommed/archive/v${pkgver}.tar.gz" 'pommed.service' )
dfname="NGPommed-${pkgver}"
sha256sums=('34d47ff34e099345c4b5f99fc4390ed741bacada11e940f18a261b4f157fb401'
            'd620b07c74e8acd243278ef768b2bd79109e0a56e890dcec17db72777d7a21e1')

build() {
  cd ${srcdir}/${dfname}
  make pommed
}

package() {
  install -Dm755 ${srcdir}/${dfname}/pommed/pommed ${pkgdir}/usr/bin/pommed
  install -Dm644 ${srcdir}/${dfname}/pommed.conf.mactel ${pkgdir}/etc/pommed.conf.mactel
  install -Dm644 ${srcdir}/${dfname}/pommed.conf.pmac ${pkgdir}/etc/pommed.conf.pmac

  # Man page
  install -Dm644 ${srcdir}/${dfname}/pommed.1 ${pkgdir}/usr/share/man/man1/pommed.1

  # Sounds
  install -Dm644 ${srcdir}/${dfname}/pommed/data/goutte.wav ${pkgdir}/usr/share/pommed/goutte.wav
  install -Dm644 ${srcdir}/${dfname}/pommed/data/click.wav ${pkgdir}/usr/share/pommed/click.wav

  # Systemd
  install -Dm644 ${srcdir}/pommed.service ${pkgdir}/usr/lib/systemd/system/pommed.service
}

# vim:set ts=2 sw=2 et:
