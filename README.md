# rockchip_drm_demos
Test demo for rk internal kernel 4.4 drm

# Build:

  copy to android project
  source build/envsetup.sh
  mm

CABC Test:

cabc_test -h

    -h or --help - this
    -a or --gamma=[float] - set pow(gamma) gamma table with gamma=f
    -m or --mode=[0 1 2 3] - cabc mode
                 0 - disabled cabc
                 1 - Predefine cabc normal mode
                 2 - Predefine cabc lowpower mode
                 3 - userspace mode
    -u or --stage_up=[0-511] - set cabc stage up
    -d or --stage_down=[0-255] - set cabc stage down
    -g or --global_dn=[0-255] - set cabc global_dn
    -p or --calc_pixel_num=[0 - 1000] - percent of number calc pixel
    -c or --crtc=[object_id] - crtc index
    -l or --list - show current cabc state
  

