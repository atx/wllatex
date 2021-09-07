project(
    'wllatex', 'c',
    version: '0.1',
    license: 'MIT',
    default_options : [
        'buildtype=release',
        'default_library=static',
        'b_sanitize=address',
    ],
)

git = find_program('git', native: true, required: false)

if not git.found()
    add_project_arguments('-DVERSION="@0@"'.format(meson.project_version()), language: 'c')
else
    git_commit_hash = run_command([git.path(), 'describe', '--always', '--tags']).stdout().strip()
    git_branch = run_command([git.path(), 'rev-parse', '--abbrev-ref', 'HEAD']).stdout().strip()
    version = '"@0@ (" __DATE__ ", branch \'@1@\')"'.format(git_commit_hash, git_branch)
    add_project_arguments('-DVERSION=@0@'.format(version), language: 'c')
endif
cc = meson.get_compiler('c')


wayland_client = dependency('wayland-client')
wayland_cursor = dependency('wayland-cursor')
xkbcommon = dependency('xkbcommon')
rt = cc.find_library('rt')


prefix = get_option('prefix')
conf_data = configuration_data()
conf_data.set('prefix', prefix)


src_files = files(
    'main.c',
    'tex.c',
    'args.c',
    'utils.c',
)

subdir('protocol')

executable(
    meson.project_name(),
    src_files,
    dependencies: [
        client_protos,
        wayland_client,
        wayland_cursor,
        rt,
        xkbcommon
    ],
    install: true,
)

