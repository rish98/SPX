/*
 * Copyright 2008 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include "cmocka.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <poll.h>
#include <math.h>
#include <stddef.h>
#include <stdbool.h>

// #include "../spx_common.h"
// #include "../spx_exchange.h"
// #include "../spx_common.h"
static void test_cmocka(void **state) {
    int fd = open("output.txt", O_RDWR|O_CREAT, 0777);
    dup2(fd, STDOUT_FILENO);
    FILE * fp;
    char *buff;
    
    sigHandler(1);
    printf("hello");
    
    read(1,buff,7);
    printf("fewfe %s\n",buff);
    assert_int_equal(1,sigHandler(1));
    assert_string_equal(buff,"hello");
}

static void test_parse(void **state) {
    int ret =parseCommand("BUY 2 Water 20 48",0);
    assert_int_equal(0,ret);

}


int main(void) {
    const struct CMUnitTest tests[] = {
        unit_test(test_cmocka),unit_test(test_parse),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
