# vim: set filetype=python :

YAJL_BUILD_PATH    = "./lib/yajl/build/yajl-2.0.5/"
TERMBOX_BUILD_PATH = "./lib/termbox/build/"

env = Environment( 
                  CC = "clang",
                  CPPPATH = ["./include/",
                             "./lib/termbox/src/",
                             YAJL_BUILD_PATH + "/include/"],
                  LIBPATH = [TERMBOX_BUILD_PATH + "/src/",
                             YAJL_BUILD_PATH + "/lib/"],
                 )

env.Program('clerk', 
            '''
            src/main.c
            src/clerk.c
            src/clerk_draw.c
            src/clerk_list.c
            src/clerk_json.c
            '''.split(),
            LIBS = ['termbox', 'yajl_s'],
            parse_flags = "-DDEBUG"
           )
