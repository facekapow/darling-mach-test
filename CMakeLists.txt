project(darling-tests)

add_darling_executable(mach-server mach-server.c)
add_darling_executable(mach-client mach-client.c)

install(TARGETS mach-server mach-client DESTINATION libexec/darling/usr/local/bin)
