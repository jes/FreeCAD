#include "PreCompiled.h"
#include "TopoShape.h"
#include "AsyncProcessHandle.h"
#include "BooleanOperation.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <vector>

#include <Base/Exception.h>
#include <Base/Console.h>

#include <Mod/Part/PartGlobal.h>

using namespace Part;

AsyncProcessHandle::AsyncProcessHandle(pid_t childPid, int resultFd)
    : pid(childPid)
    , fd(resultFd)
{
    valid.store(true);
}

AsyncProcessHandle::AsyncProcessHandle(AsyncProcessHandle&& other) noexcept
    : pid(other.pid)
    , fd(other.fd)
    , valid(other.valid.load())
{
    // Invalidate the other object
    other.pid = -1;
    other.fd = -1;
    other.valid.store(false);
}

AsyncProcessHandle& AsyncProcessHandle::operator=(AsyncProcessHandle&& other) noexcept
{
    if (this != &other) {
        // Transfer ownership
        pid = other.pid;
        fd = other.fd;
        valid = other.valid.load();

        // Invalidate the other object
        other.pid = -1;
        other.fd = -1;
        other.valid.store(false);
    }
    return *this;
}

AsyncProcessHandle::~AsyncProcessHandle()
{
    if (isValid()) {
        // Attempt to abort if we haven't joined
        try {
            abort();
            waitpid(pid, nullptr, 0);
        } catch (...) {
            // Ignore exceptions in destructor
        }
    }
    
    if (fd >= 0) {
        close(fd);
    }
}

bool AsyncProcessHandle::isValid()
{
    return valid.load();
}

void AsyncProcessHandle::abort()
{
    if (!isValid()) {
        Base::Console().Error("Aborting process: not valid\n");
        return;
    }

    Base::Console().Error("Aborting process: valid\n");

    if (pid > 0) {
        Base::Console().Error("Aborting process: pid > 0 (%d)\n", pid);
        kill(pid, SIGTERM);
        // Give it a moment to terminate gracefully
        usleep(100000);  // 100ms
        
        // Force kill if still running
        if (kill(pid, 0) == 0) {
            kill(pid, SIGKILL);
        }
    }
}

TopoShape AsyncProcessHandle::join()
{
    if (!isValid()) {
        throw Base::RuntimeError("Process is not valid");
    }

    // Read result using BooleanOperation's protocol
    bool isError;
    std::string result = BooleanOperation::readResult(fd, isError);

    // Wait for child process first to ensure clean exit
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        throw Base::RuntimeError("Error waiting for child process");
    }

    valid.store(false);

    // Check exit status before processing any data
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw Base::RuntimeError("Child process failed, exit status: " + std::to_string(WEXITSTATUS(status)));
    }
    
    if (isError) {
        throw Base::RuntimeError(result);
    }

    // Process shape data
    std::istringstream str(result);
    Part::TopoShape shape;
    shape.importBinary(str);
    return shape;
}
