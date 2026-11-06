service_conf = configuration_data()
service_conf.set('bindir', get_option('prefix') / get_option('bindir'))

configure_file(
  input: 'os.nodalix.Store.SearchProvider.service.in',
  output: 'os.nodalix.Store.SearchProvider.service',
  configuration: service_conf,
  install_dir: get_option('datadir') / 'dbus-1' / 'services'
)

configure_file(
  input: 'os.nodalix.Store.service.in',
  output: 'os.nodalix.Store.service',
  configuration: service_conf,
  install_dir: get_option('datadir') / 'dbus-1' / 'services'
)