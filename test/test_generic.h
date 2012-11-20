/*
 * test_generic.h
 *
 *  Created on: 18 Sep 2012
 *      Author: lukasz.forynski
 *
 *  @brief:  This is a helper file for testing. It provides entry point for tests
 *  (definition of main + Catch declarations).
 *  Also signals are handled, so that segmentation violations or other faults didn't cause
 *  tests to hang. Instead - they will exit with the -SIGxxx value.
 */

#ifndef TEST_GENERIC_H_
#define TEST_GENERIC_H_

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#define REQUIRE_NOT_NULL( expr ) REQUIRE((expr) != (void*)NULL)
#define REQUIRE_NULL( expr ) REQUIRE((expr) == (void*)NULL)
#ifndef SIGBREAK
#define SIGBREAK SIGTSTP
#endif

#include <signal.h>
#include <exception>

/**
 * @brief: signal handler.
 *   This function gets when registered signal gets received by the the application.
 *   It prints info about the signal and current test, which should help with debugging.
 *   Should not be used directly.
 * @param sig: numerical value of the signal, as defined in signal.h
 * @return: Does not return. Exists with -sig (negative value of the signal numerical value)
 */
void handle_signal(int sig)
{
    printf("\n\n=================\n\n");

    switch (sig)
    {
    case SIGINT:
        printf("Interactive attention");
        break;
    case SIGILL:
        printf("Illegal instruction");
        break;
    case SIGFPE:
        printf("Floating point error");
        break;
    case SIGSEGV:
        printf("Segmentation violation");
        break;
    case SIGTERM:
        printf("Termination request");
        break;
    case SIGBREAK:
        printf("Control-break");
        break;
    case SIGABRT:
        printf("Abnormal termination (abort)");
        break;
    default:
        printf("other signal received: %d", sig);
        break;
    }

    // print info about current test
    Catch::IResultCapture& result = Catch::getCurrentContext().getResultCapture();
    printf("\n\nwhile executing test: %s\n", result.getCurrentTestName().c_str());
    printf("(last successful check was in %s,", result.getLastResult()->getFilename().c_str());
    printf("line: %d)\n\n", static_cast<int>(result.getLastResult()->getLine()));
    Catch::cleanUp();

    exit(-sig);
}

/**
 * @brief: main
 * Entry point for the test application (test-suite).
 * Registers signal handler for various signals and provides entry point
 * for Catch test environment.
 */
int main(int argc, char* const argv[])
{
    int result = -1;

    signal(SIGSEGV, handle_signal);
    signal(SIGABRT, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGILL, handle_signal);
    signal(SIGFPE, handle_signal);
    signal(SIGSEGV, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGBREAK, handle_signal);
    signal(SIGABRT, handle_signal);

    try
    {
        result = Catch::Main(argc, argv);

    }
    catch (std::exception& e)
    {
        printf("Unexpected exception: %s\n", e.what());
    }

    return result;
}

#endif /* TEST_GENERIC_H_ */
