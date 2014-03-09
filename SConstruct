env = Environment( 
                  CC = "clang",
                  CPPPATH = ["./lib/termbox/src/", "./include/", "./lib/yajl/src/api"],
                  LIBPATH = ["./lib/termbox/build/src", "./lib/yajl/build/yajl-2.0.5/lib/"],
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
            #parse_flags = "-DDEBUG"
           )
