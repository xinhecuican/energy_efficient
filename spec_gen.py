import argparse
import os
import sys

elf_suffix = "_base.rv64"
elf_prefix = "run/run_base_refrate_rv64.0000/"

# (filelist, arguments) information for each benchmark
# filelist[0] should always be the binary file
def get_spec_info():
  return {
  "perlbench_r": (
    [
      "${SPEC}/benchspec/CPU/500.perlbench_r/" + elf_prefix + "perlbench_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/500.perlbench_r/" + elf_prefix + "checkspam.pl",
      "dir lib /home/lizilin/projects/cpu2017/benchspec/CPU/500.perlbench_r/data/all/input/lib"
    ],
    ["-I", "lib", "checkspam.pl", "2500", "5", "25", "11", "150", "1", "1", "1", "1" ],
    [ "int", "ref" ]
  ),
  "perlbench_s": (
    [
      "${SPEC}/benchspec/CPU/600.perlbench_s/" + elf_prefix + "perlbench_s" + elf_suffix,
      "${SPEC}/benchspec/CPU/600.perlbench_s/" + elf_prefix + "checkspam.pl",
      "dir lib /home/lizilin/projects/cpu2017/benchspec/CPU/600.perlbench_s/run/run_base_refrate_rv64.0000/lib"
    ],
    ["-I", "lib", "checkspam.pl", "2500", "5", "25", "11", "150", "1", "1", "1", "1" ],
    [ "int", "ref" ]
  ),
  "gcc_r": (
    [
      "${SPEC}/benchspec/CPU/502.gcc_r/" + elf_prefix + "gcc_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/502.gcc_r/" + elf_prefix + "gcc-pp.c"
    ],
    [ "gcc-pp.c", "-O3", "-finline-limit=0", "-fif-conversion", "-fif-conversion2", "-o gcc-pp.opts-O3_-finline-limit_0_-fif-conversion_-fif-conversion2.s" ],
    [ "snt", "ref" ]
  ),
  "gcc_s": (
    [
      "${SPEC}/benchspec/CPU/602.gcc_s/" + elf_prefix + "gcc_s" + elf_suffix,
      "${SPEC}/benchspec/CPU/602.gcc_s/" + elf_prefix + "gcc-pp.c"
    ],
    [ "gcc-pp.c", "-O5", "-finline-limit=1000", "-fselective-scheduling", "-fselective-scheduling2", "-o", "gcc-pp.opts-O5_-finline-limit_1000_-fselective-scheduling_-fselective-scheduling2.out" ],
    [ "int", "ref" ]
  ),
  "bwaves_r": (
    [
      "${SPEC}/benchspec/CPU/503.bwaves_r/" + elf_prefix + "bwaves_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/503.bwaves_r/" + elf_prefix + "bwaves_3.in"
    ],
    ["bwave_3.in"],
    [ "fp", "ref" ]
  ),
  "mcf_r": (
    [
      "${SPEC}/benchspec/CPU/505.mcf_r/" + elf_prefix + "mcf_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/505.mcf_r/" + elf_prefix + "inp.in"
    ],
    [ "inp.in" ],
    [ "int", "ref" ]
  ),
  "mcf_s": (
    [
      "${SPEC}/benchspec/CPU/605.mcf_s/" + elf_prefix + "mcf_s" + elf_suffix,
      "${SPEC}/benchspec/CPU/605.mcf_s/" + elf_prefix + "inp.in"
    ],
    [ "inp.in" ],
    [ "int", "ref" ]
  ),
  "cactuBSSN_r": (
    [
      "${SPEC}/benchspec/CPU/507.cactuBSSN_r/" + elf_prefix + "cactuBSSN_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/507.cactuBSSN_r/" + elf_prefix + "spec_ref.par"
    ],
    [ "spec_ref.par" ],
    [ "int", "ref" ]
  ),
  "namd_r": (
    [
      "${SPEC}/benchspec/CPU/508.namd_r/" + elf_prefix + "namd_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/508.namd_r/" + elf_prefix + "apoa1.input"
    ],
    ["--input", "apoa1.input", "--iterations", "65", "--output", "apoa1.ref.output"],
    [ "int", "ref" ]
  ),
  "parest_r": (
    [
      "${SPEC}/benchspec/CPU/510.parest_r/" + elf_prefix + "parest_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/510.parest_r/" + elf_prefix + "ref.prm"
    ],
    [ "ref.prm" ],
    [ "int", "ref" ]
  ),
  "povray_r": (
    [
      "${SPEC}/benchspec/CPU/511.povray_r/" + elf_prefix + "povray_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/511.povray_r/" + elf_prefix + "SPEC-benchmark-ref.ini"
    ],
    [ "SPEC-benchmark-ref.ini" ],
    [ "int", "ref" ]
  ),
  "lbm_r": (
    [
      "${SPEC}/benchspec/CPU/519.lbm_r/" + elf_prefix + "lbm_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/519.lbm_r/" + elf_prefix + "100_100_130_ldc.of"
    ],
    ["3000", "reference.dat", "0", "0", "100_100_130_ldc.of"],
    ["int", "ref"]
  ),
  "omnetpp_r": (
    [
      "${SPEC}/benchspec/CPU/520.omnetpp_r/" + elf_prefix + "omnetpp_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/520.omnetpp_r/" + elf_prefix + "omnetpp.ini",
      "dir ned /home/lizilin/projects/cpu2017/benchspec/CPU/520.omnetpp_r/run/run_base_refrate_rv64.0000/ned"
    ],
    [ "-c", "General", "-r", "0" ],
    [ "fp", "ref" ]
  ),
  "wrf_r": (
    [
      "${SPEC}/benchspec/CPU/521.wrf_r/" + elf_prefix + "wrf_r" + elf_suffix,
    ],
    [  ],
    [ "fp", "ref" ]
  ),
  "xalancbmk_r": (
    [
      "${SPEC}/benchspec/CPU/523.xalancbmk_r/" + elf_prefix + "xalancbmk_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/523.xalancbmk_r/" + elf_prefix + "t5.xml",
      "${SPEC}/benchspec/CPU/523.xalancbmk_r/" + elf_prefix + "xalanc.xsl",
      "${SPEC}/benchspec/CPU/523.xalancbmk_r/" + elf_prefix + "100mb.xsd",
    ],
    [ "-v", "t5.xml", "xalanc.xsl" ],
    [ "fp", "ref" ]
  ),
  "x264_r": (
    [
      "${SPEC}/benchspec/CPU/525.x264_r/" + elf_prefix + "x264_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/525.x264_r/" + elf_prefix + "BuckBunny.yuv"
    ],
    [ "--pass", "1", "--stats", "x264_stats.log", "--bitrate", "1000", "--frames", "1000", "-o", "BuckBunny_New.264", "BuckBunny.yuv", "1280x720"],
    [ "fp", "ref" ]
  ),
  "blender_r": (
    [
      "${SPEC}/benchspec/CPU/526.blender_r/" + elf_prefix + "blender_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/526.blender_r/" + elf_prefix + "sh3_no_char.blend"
    ],
    [ "sh3_no_char.blend", "--render-output", "sh3_no_char_", "--threads", "1", "-b", "-F", "RAWTGA", "-s 849", "-e", "849", "-a"],
    [ "fp", "ref" ]
  ),
  "cam4_r": (
    [
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "cam4_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "drv_in",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "atm_in",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "dust1_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "dust2_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "dust3_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "dust4_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "ocphi_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "ocpho_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "ozone_1.9x2.5_L26_2000clim_c091112.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "ssam_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "sscm_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "sulfate_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "drv_flds_in",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "bcphi_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "bcpho_camrt_c080918.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "USGS-gtopo30_1.9x2.5_remap_c050602.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "abs_ems_factors_fastvx.c030508.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "cami_0000-01-01_1.9x2.5_L26_APE_c080203.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "clim_p_trop.nc",
      "${SPEC}/benchspec/CPU/527.cam4_r/" + elf_prefix + "aero_1.9x2.5_L26_2000clim_c091112.nc"



    ],
    [  ],
    [ "fp", "ref" ]
  ),
  "deepsjeng_r": (
    [
      "${SPEC}/benchspec/CPU/531.deepsjeng_r/" + elf_prefix + "deepsjeng_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/531.deepsjeng_r/" + elf_prefix + "ref.txt"
    ],
    [ "ref.txt" ],
    [ "int", "ref" ]
  ),
  "leela_r": (
    [
      "${SPEC}/benchspec/CPU/541.leela_r/" + elf_prefix + "leela_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/541.leela_r/" + elf_prefix + "ref.sgf"
    ],
    [ "ref.sgf" ],
    [ "int", "ref" ]
  ),
  "nab_r": (
    [
      "${SPEC}/benchspec/CPU/544.nab_r/" + elf_prefix + "nab_r" + elf_suffix,
      "dir lam0 /home/lizilin/projects/cpu2017/benchspec/CPU/544.nab_r/run/run_base_refrate_rv64.0000/1am0",
    ],
    [ "lam0", "1122214447", "122" ],
    [ "int", "ref" ]
  ),
  "exchange2_r": (
    [
      "${SPEC}/benchspec/CPU/548.exchange2_r/" + elf_prefix + "exchange2_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/548.exchange2_r/" + elf_prefix + "puzzles.txt",
    ],
    [ "6" ],
    [ "int", "ref" ]
  ),
  "exchange2_s": (
    [
      "${SPEC}/benchspec/CPU/648.exchange2_s/" + elf_prefix + "exchange2_s" + elf_suffix,
      "${SPEC}/benchspec/CPU/648.exchange2_s/" + elf_prefix + "puzzles.txt",
    ],
    [ "6" ],
    [ "int", "ref" ]
  ),
  "fotonik3d_r": (
    [
      "${SPEC}/benchspec/CPU/549.fotonik3d_r/" + elf_prefix + "fotonik3d_r" + elf_suffix
    ],
    [ ],
    [ "int", "ref" ]
  ),
  "roms_r": (
    [
      "${SPEC}/benchspec/CPU/554.roms_r/" + elf_prefix + "roms_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/554.roms_r/" + elf_prefix + "ocean_benchmark2.in.x"
    ],
    ["ocean_benchmark2.in.x"],
    ["int","ref"]
  ),
  "xz_r": (
    [
      "${SPEC}/benchspec/CPU/557.xz_r/" + elf_prefix + "xz_r" + elf_suffix,
      "${SPEC}/benchspec/CPU/557.xz_r/" + elf_prefix + "cld.tar.xz"
    ],
    [ "cld.tar.xz", "160", "19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474", "59796407", "61004416", "6"],
    [ "int", "ref" ]
  ),
  "xz_s": (
    [
      "${SPEC}/benchspec/CPU/657.xz_s/" + elf_prefix + "xz_s" + elf_suffix,
      "${SPEC}/benchspec/CPU/657.xz_s/" + elf_prefix + "cpu2006docs.tar.xz"
    ],
    [ "cpu2006docs.tar.xz", "6643", "055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae", "1036078272", "1111795472", "4"],
    [ "int", "ref" ]
  ),
  "imagick_r": (
    [
        "${SPEC}/benchspec/CPU/538.imagick_r/" + elf_prefix + "imagick_r" + elf_suffix,
        "${SPEC}/benchspec/CPU/538.imagick_r/" + elf_prefix + "refrate_input.tga"
    ],
    ["-limit", "disk", "0", "refrate_input.tga", "-edge", "41", "-resample", "181%", "-emboss", "31", "-colorspace", "YUV", "-mean-shift", "19x19+15%", "-resize", "30%", "refrate_output.tga"],
    [ "int", "ref" ]
  ),
  "specrand_fs": (
    [
      "${SPEC}/benchspec/CPU/996.specrand_fs/" + elf_prefix + "specrand_fs" + elf_suffix,
      "${SPEC}/cpu2006_run_dir/gcc/c-typeck.i"
    ],
    [ "c-typeck.i", "-o", "c-typeck.s" ],
    [ "int", "ref" ]
  ),
  "specrand_fr": (
    [
      "${SPEC}/benchspec/CPU/997.specrand_fr/" + elf_prefix + "specrand_fr" + elf_suffix,
      "${SPEC}/cpu2006_run_dir/GemsFDTD/ref.in",
      "${SPEC}/cpu2006_run_dir/GemsFDTD/sphere.pec",
      "${SPEC}/cpu2006_run_dir/GemsFDTD/yee.dat"
    ],
    [],
    [ "fp", "ref" ]
  ),
  "specrand_is": (
    [
      "${SPEC}/benchspec/CPU/998.specrand_is/" + elf_prefix + "specrand_is" + elf_suffix,
      "${SPEC}/cpu2006_run_dir/gobmk/13x13.tst",
      "dir games /nfs/home/share/xs-workloads/spec/spec-all/cpu2006_run_dir/gobmk/games",
      "dir golois /nfs/home/share/xs-workloads/spec/spec-all/cpu2006_run_dir/gobmk/golois"
    ],
    [ "--quiet", "--mode", "gtp", "<", "13x13.tst" ],
    [ "int", "ref" ]
  ),
  "specrand_ir": (
    [
      "${SPEC}/benchspec/CPU/999.specrand_ir/" + elf_prefix + "specrand_ir" + elf_suffix,
      "${SPEC}/cpu2006_run_dir/gobmk/nngs.tst",
      "dir games /nfs/home/share/xs-workloads/spec/spec-all/cpu2006_run_dir/gobmk/games",
      "dir golois /nfs/home/share/xs-workloads/spec/spec-all/cpu2006_run_dir/gobmk/golois"
    ],
    [ "--quiet", "--mode", "gtp", "<", "nngs.tst" ],
    [ "int", "ref" ]
  ),
}

default_files = [
  "dir /bin 755 0 0",
  "dir /etc 755 0 0",
  "dir /dev 755 0 0",
  "dir /lib 755 0 0",
  "dir /proc 755 0 0",
  "dir /sbin 755 0 0",
  "dir /sys 755 0 0",
  "dir /tmp 755 0 0",
  "dir /usr 755 0 0",
  "dir /mnt 755 0 0",
  "dir /usr/bin 755 0 0",
  "dir /usr/lib 755 0 0",
  "dir /usr/sbin 755 0 0",
  "dir /var 755 0 0",
  "dir /var/tmp 755 0 0",
  "dir /root 755 0 0",
  "dir /var/log 755 0 0",
  "",
  "nod /dev/console 644 0 0 c 5 1",
  "nod /dev/null 644 0 0 c 1 3",
  "",
  "# libraries",
  "file /lib/ld-linux-riscv64-lp64d.so.1 ${RISCV}/sysroot/lib/ld-linux-riscv64-lp64d.so.1 755 0 0",
  "file /lib/libc.so.6 ${RISCV}/sysroot/lib/libc.so.6 755 0 0",
  "file /lib/libresolv.so.2 ${RISCV}/sysroot/lib/libresolv.so.2 755 0 0",
  "file /lib/libm.so.6 ${RISCV}/sysroot/lib/libm.so.6 755 0 0",
  "file /lib/libdl.so.2 ${RISCV}/sysroot/lib/libdl.so.2 755 0 0",
  "file /lib/libpthread.so.0 ${RISCV}/sysroot/lib/libpthread.so.0 755 0 0",
  "file /lib/libgfortran.so.5 ${RISCV}/sysroot/lib/libgfortran.so.5 755 0 0",
  "file /lib/libgcc_s.so.1 ${RISCV}/sysroot/lib/libgcc_s.so.1 755 0 0",
  "file /lib/libstdc++.so.6 ${RISCV}/sysroot/lib/libstdc++.so.6 755 0 0",
  "file /lib/libgomp.so.1 ${RISCV}/sysroot/lib/libgomp.so.1 755 0 0",
  "# busybox",
  "file /bin/busybox ${RISCV_ROOTFS_HOME}/rootfsimg/build/busybox 755 0 0",
  "file /etc/inittab ${RISCV_ROOTFS_HOME}/rootfsimg/inittab-spec 755 0 0",
  "slink /init /bin/busybox 755 0 0",
  "",
  "# SPEC common",
  "dir /spec_common 755 0 0",
  "file /spec_common/before_workload ${SPEC}/before_workload 755 0 0",
  "file /spec_common/trap ${SPEC}/trap_new 755 0 0",
  "",
  "# SPEC",
  "dir /spec 755 0 0",
  "file /spec/run.sh ${RISCV_ROOTFS_HOME}/rootfsimg/run.sh 755 0 0"
]

def traverse_path(path, stack=""):
  all_dirs, all_files = [], []
  for item in os.listdir(path):
    item_path = os.path.join(path, item)
    item_stack = os.path.join(stack, item)
    if os.path.isfile(item_path):
      all_files.append(item_stack)
    else:
      all_dirs.append(item_stack)
      sub_dirs, sub_files = traverse_path(item_path, item_stack)
      all_dirs.extend(sub_dirs)
      all_files.extend(sub_files)
  return (all_dirs, all_files)

def generate_initramfs(specs):
  lines = default_files.copy()
  for spec in specs:
    spec_files = get_spec_info()[spec][0]
    for i, filename in enumerate(spec_files):
      if len(filename.split()) == 1:
        # print(f"default {filename} to file 755 0 0")
        basename = filename.split("/")[-1]
        filename = f"file /spec/{basename} {filename} 755 0 0"
        lines.append(filename)
      elif len(filename.split()) == 3:
        node_type, name, path = filename.split()
        if node_type != "dir":
          print(f"unknown filename: {filename}")
          continue
        all_dirs, all_files = traverse_path(path)
        lines.append(f"dir /spec/{name} 755 0 0")
        for sub_dir in all_dirs:
          lines.append(f"dir /spec/{name}/{sub_dir} 755 0 0")
        for file in all_files:
          lines.append(f"file /spec/{name}/{file} {path}/{file} 755 0 0")
      else:
        print(f"unknown filename: {filename}")
  with open("initramfs-spec.txt", "w") as f:
    f.writelines(map(lambda x: x + "\n", lines))


def generate_run_sh(specs, withTrap=False):
  lines =[ ]
  lines.append("#!/bin/sh")
  lines.append("echo '===== Start running SPEC2006 ====='")
  for spec in specs:
    spec_bin = get_spec_info()[spec][0][0].split("/")[-1]
    spec_cmd = " ".join(get_spec_info()[spec][1])
    lines.append(f"echo '======== BEGIN {spec} ========'")
    lines.append("set -x")
    lines.append(f"md5sum /spec/{spec_bin}")
    lines.append("date -R")
    if withTrap:
      lines.append("/spec_common/before_workload")
    lines.append(f"cd /spec && ./{spec_bin} {spec_cmd}")
    if withTrap:
      lines.append("/spec_common/trap")
    lines.append("date -R")
    lines.append("set +x")
    lines.append(f"echo '======== END   {spec} ========'")
  lines.append("echo '===== Finish running SPEC2006 ====='")
  with open("run.sh", "w") as f:
    f.writelines(map(lambda x: x + "\n", lines))

def generate_build_scripts(specs, withTrap=False, spec_gen=__file__):
  lines = []
  lines.append("#!/bin/sh")
  lines.append("set -x")
  lines.append("set -e")
  spike_dir, linux_dir = "../../riscv-pk", "../../riscv-linux"
  lines.append("mkdir -p spec_images")
  for spec in specs:
    target_dir = f"spec_images/{spec}"
    lines.append(f"mkdir -p {target_dir}")
    extra_args = ""
    if withTrap:
      extra_args += " --checkpoints"
    extra_args += f" --elf-suffix {elf_suffix}"
    lines.append(f"python3 {spec_gen} {spec}{extra_args}")
    lines.append(f"make -s -C {spike_dir} clean && make -s -C {spike_dir} -j100")
    bbl_elf = f"{spike_dir}/build/bbl"
    linux_elf = f"{linux_dir}/vmlinux"
    spec_elf = get_spec_info()[spec][0][0]
    bbl_bin = f"{spike_dir}/build/bbl.bin"
    for f in [bbl_elf, linux_elf, spec_elf]:
      filename = os.path.basename(f)
      lines.append(f"riscv64-unknown-linux-gnu-objdump -d {f} > {target_dir}/{filename}.txt")
    for f in [bbl_elf, linux_elf, spec_elf, bbl_bin]:
      lines.append(f"cp {f} {target_dir}")
  with open("build.sh", "w") as f:
    f.writelines(map(lambda x: x + "\n", lines))

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='CPU CPU2017 ramfs scripts')
  parser.add_argument('benchspec', nargs='*', help='selected benchmarks')
  parser.add_argument('--elf-suffix', '-s',
                      help='elf suffix (default: _base.riscv64-linux-gnu-gcc-9.3.0)')
  parser.add_argument('--checkpoints', action='store_true',
                      help='checkpoints mode (with before_workload and trap)')
  parser.add_argument('--scripts', action='store_true',
                      help='generate build scripts for spec ramfs')

  args = parser.parse_args()

  if args.elf_suffix is not None:
    elf_suffix = args.elf_suffix

  # parse benchspec
  benchspec = []
  spec_info = get_spec_info()
  for s in args.benchspec:
    if s in spec_info:
      benchspec.append(s)
    else:
      benchspec += [k for k in spec_info.keys() if set(s.split(",")) <= set(spec_info[k][2])]
  benchspec = sorted(set(benchspec))
  print(f"All {len(benchspec)} selected benchspec: {' '.join(benchspec)}")

  if args.scripts:
    generate_build_scripts(benchspec, args.checkpoints)
  else:
    generate_initramfs(benchspec)
    generate_run_sh(benchspec, args.checkpoints)
