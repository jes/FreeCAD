#ifndef GUI_COMPUTATION_DIALOG_H
#define GUI_COMPUTATION_DIALOG_H

#include <QProgressDialog>
#include <atomic>
#include <functional>
#include "MainWindow.h"
#include "Base/ProgressIndicator.h"

#include "FCGlobal.h"

namespace Gui {

class GuiExport ComputationDialog : public Base::ProgressIndicator {
public:
    ComputationDialog() : aborted(false) {}

    // Message_ProgressIndicator interface implementation
    bool UserBreak() override;
    void Show(float position, bool isForce) override;
    void abort();
    void run(std::function<void()> func);

private:
    std::atomic<bool> aborted;
};

} // namespace Gui

#endif // GUI_COMPUTATION_DIALOG_H
