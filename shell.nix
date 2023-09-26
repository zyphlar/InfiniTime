{ pkgs ? import <nixpkgs> {} }:

with pkgs; let
  py4McuBoot = python3.withPackages (ps: with ps; [
    cbor
    intelhex
    click
    cryptography
  ]);
in mkShell {
  packages = [
    gcc-arm-embedded-10
    nrf5-sdk
    cmake
    nodePackages.lv_font_conv
    lv_img_conv
    py4McuBoot
    clang-tools
    SDL2
    libpng
    python3Packages.adafruit-nrfutil
  ];

  ARM_NONE_EABI_TOOLCHAIN_PATH="${gcc-arm-embedded-10}";
  NRF5_SDK_PATH="${nrf5-sdk}/share/nRF5_SDK";
  CMAKE_BUILD_TYPE="Release";
  BUILD_DFU=1;
  BUILD_RESOURCES=1;
  TARGET_DEVICE="PINETIME";

}
