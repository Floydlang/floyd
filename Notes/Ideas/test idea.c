Thank's for you input so far! You've taught me stuff!

This feature was just an idea but I believe it has some merit.

The goal is to be able to test functions without:
1) having to alter them
2) having to alter their clients
3) introducing code complexity in the function-under-test
4) affect program's runtime behaviour in release builds.

Introducing interfaces for the SOLE purpose of testing often add accidental complexity to the function-under-test.

Contrived example in C of what I was thinking of:


    #include <stdio.h>

    float multiply(float a, float b){
        return a * b;
    }

    float square(float a){
        return multiply(a, a)
    }

    int main(int argc, char** argv){
        printf("Hello World\n");
        printf("3 x 8 = %f\n", multiply(3.0f, 8.0f));
        printf("3^2 = %f\n", square(3));
        return 0;
    }


    //  TESTS

    test(multiply) {
        assert(multiply(0.0f, 0.0f) == 0.0f);
        assert(multiply(9.0f, 3.0f) == 27.0f);
    }

    test(square) {
        //  Fake function, or this test won't compile. Alternatively you can map multiply to an existing function.
        float multiply(float a, float b){
            return 10;
        }

        assert(square(3.0f) == 10.0f);
        assert(square(9.0f, 3.0f) == 10.0f);
    }

    test(main) {
        char out_s[1024 + 1];

        //  Fake function
        int printf(const char format[], ...){
            char temp[1024 + 1];
            sprintf(temp, format, ...)
            strcat(out_s, temp);
        }

        //  Fake function
        float multiply(float a, float b){
            return 100;
        }

        //  Fake function
        float square(float a, float b){
            return 1000;
        }

        int result = main(0, NULL);
        assert(strcmp(out_s, "Hello World\n3 x 8 = 100.0\n3^2 = 100.0\n") == 0);
        assert(result = 0);
    }



