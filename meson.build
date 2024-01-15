project('libhashtable', 'c',
        default_options: 'c_std=c99',
        version: '0.6.4',
        license: 'MIT')

if host_machine.cpu_family() == 'x86'
  add_project_arguments('-DCPU_32_BIT', language: 'c')
elif host_machine.cpu_family() == 'x86_64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
elif host_machine.cpu_family() == 'aarch64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
elif host_machine.cpu_family() == 'arc'
  add_project_arguments('-DCPU_32_BIT', language: 'c')
elif host_machine.cpu_family() == 'arm'
  add_project_arguments('-DCPU_32_BIT', language: 'c')
elif host_machine.cpu_family() == 'ia64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
elif host_machine.cpu_family() == 'loongarch64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
elif host_machine.cpu_family() == 'mips'
  add_project_arguments('-DCPU_32_BIT', language: 'c')
elif host_machine.cpu_family() == 'mips64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
elif host_machine.cpu_family() == 'ppc'
  add_project_arguments('-DCPU_32_BIT', language: 'c')
elif host_machine.cpu_family() == 'ppc64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
elif host_machine.cpu_family() == 'sparc'
  add_project_arguments('-DCPU_32_BIT', language: 'c')
elif host_machine.cpu_family() == 'sparc64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
elif host_machine.cpu_family() == 'wasm32'
  add_project_arguments('-DCPU_32_BIT', language: 'c')
elif host_machine.cpu_family() == 'wasm64'
  add_project_arguments('-DCPU_64_BIT', language: 'c')
endif

add_project_arguments('-D_DEFAULT_SOURCE', language: 'c')
add_project_arguments('-DXOPEN=600', language: 'c')

inc = include_directories('include')

subdir('src')
subdir('include')
subdir('test')
