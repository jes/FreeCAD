#include "ComputationDialog.h"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>

#include <Base/ProgressIndicator.h>
#include <Gui/ProgressBar.h>

namespace Gui {

void ComputationDialog::Show(float position, bool isForce) {
    (void)isForce;

    static int lastPct = -1;
    int pct = position * 100;

    // only setPercent() if it's changed, otherwise it makes the UI laggy
    // with thousands of no-op calls
    if (pct != lastPct) {
        Gui::SequencerBar::instance()->setPercent(pct);
        lastPct = pct;
    }
}

void ComputationDialog::abort() {
    aborted.store(true);
}

void ComputationDialog::run(std::function<void()> func) {
    std::atomic<bool> computationDone(false);
    std::mutex mutex;
    std::condition_variable cv;
    std::exception_ptr threadException;

    // TODO: you're meant to initialise the SequencerBar with something like this:
    //   Base::SequencerLauncher seq("Computation", 100);
    // but for some reason when I do that it locks up the UI?
    // If we could make that work then we could remove the hide() call below
    // and delete the method from SequencerBar.

    Base::ProgressIndicator::setInstance(this);

    try {
        func();
    } catch (...) {
        threadException = std::current_exception();
    }

    computationDone.store(true);

    cv.notify_one();

    Base::ProgressIndicator::resetInstance();

    Gui::SequencerBar::instance()->hide();

    // Re-throw any exception that occurred in the thread
    if (threadException) {
        std::rethrow_exception(threadException);
    }
}

bool ComputationDialog::UserBreak() {
    return aborted.load() || Gui::SequencerBar::instance()->wasCanceled();
}

} // namespace Gui