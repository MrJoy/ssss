{ pkgs }:

pkgs.stdenv.mkDerivation {
  pname = "ssss";
  version = "0.5.7";

  src = ./.;

  nativeBuildInputs = [ pkgs.installShellFiles ];

  buildInputs = [ pkgs.gmp ];

  preBuild = ''
    sed -e s@/usr/@$out/@g -i Makefile
    cp ssss.manpage.xml ssss.1
    mkdir -p $out/bin
    echo -e 'install:\n\tcp ssss-combine ssss-split '"$out"'/bin' >>Makefile
  '';

  postInstall = ''
    installManPage ssss.1
  '';

  meta = with pkgs.lib; {
    description = "Shamir Secret Sharing Scheme";
    homepage = "http://point-at-infinity.org/ssss/";
    license = licenses.gpl2Plus;
    platforms = platforms.unix;
  };
}

