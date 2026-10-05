#ifdef _WIN32
#include <crtdbg.h>
#include <cstdlib>
#include <cstdio>
#include <exception>
namespace {
struct NativeDiagnostics {
    NativeDiagnostics() {
        // Unattended failures emit evidence instead of waiting for a CRT dialog.
        _set_error_mode(_OUT_TO_STDERR);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        std::set_terminate([] {
            if (auto exception = std::current_exception()) {
                try {
                    std::rethrow_exception(exception);
                } catch (const std::exception &error) {
                    std::fprintf(stderr, "Unexpected termination: %s\n", error.what());
                } catch (...) {
                    std::fputs("Unexpected termination: nonstandard exception\n", stderr);
                }
            } else
                std::fputs("Unexpected termination without an exception\n", stderr);
            std::_Exit(99);
        });
    }
} diagnostics;
} // namespace
#endif
