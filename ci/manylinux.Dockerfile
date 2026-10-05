# Pinned Linux build image: AlmaLinux 8 with glibc 2.28 and GCC 14
# (gcc-toolset-14), so the plugin loads on any distribution with glibc 2.28+.
# CI and local builds use the same image:
#
#   docker build --platform linux/amd64 -t vcmp-lua-build -f ci/manylinux.Dockerfile ci
#   docker run --rm --platform linux/amd64 -v "$PWD:/src" -w /src vcmp-lua-build ci/build-linux.sh
FROM quay.io/pypa/manylinux_2_28_x86_64:2026.10.03-1@sha256:39df0042d5cc900b085aa25a0659368b42a0006c54c474299b785b44c1b4ff82

# perl-IPC-Cmd, perl-Time-Piece: OpenSSL's Configure. bison, flex: libpq.
# patch: cmake/deps. The image already has Python 3.12 (meson needs 3.7+);
# AlmaLinux's python3 package would replace it with 3.6.
RUN dnf install -y \
        bison \
        flex \
        git \
        patch \
        perl-IPC-Cmd \
        perl-Time-Piece \
        pkgconf \
        unzip \
        zip \
    && dnf clean all \
    && pipx install ninja==1.13.2

# vcpkg uses the image's pinned cmake, ninja and git instead of downloading its own.
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
