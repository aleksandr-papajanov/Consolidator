#include "c74_min.h"

#include "ApproximatorSupport.h"

#include <mutex>
#include <optional>
#include <string>
#include <thread>

using namespace c74::min;

class ConsolidatorApproximator : public object<ConsolidatorApproximator> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ curve approximator." };
    MIN_TAGS{ "audio, eq, optimizer" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> input_curve{
        this,
        "(list) target difference curve in dB"
    };

    inlet<> baseline_curve_in{
        this,
        "(list) current EQ curve in dB"
    };

    inlet<> commands{
        this,
        "(anything) commands: capture, fit"
    };

    outlet<> commands_out{
        this,
        "(anything) EQ commands"
    };

    outlet<> status_out{
        this,
        "(symbol) status: capturing, processing, done, error"
    };

    outlet<> debug_out{
        this,
        "(anything) debug info"
    };

    enum class Status {
        idle,
        capturing,
        processing,
        done,
        error,
    };

    queue<> fit_delivery{
        this,
        MIN_FUNCTION {
            deliver_fit_result();
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Receive target or baseline curve",
        MIN_FUNCTION {
            if (inlet == 0) {
                curve_store.set_target(args);
            }
            else if (inlet == 1) {
                curve_store.set_baseline(args);
            }

            return {};
        }
    };

    message<> capture_message{
        this,
        "capture",
        "Freeze the latest target curve for fitting",
        MIN_FUNCTION {
            bool join_worker = false;
            {
                std::lock_guard<std::mutex> lock(fit_mutex_);
                if (fit_running_) {
                    ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
                    outputs.error("fit_in_progress");
                    return {};
                }

                if (fit_worker_.joinable()) {
                    join_worker = true;
                }
            }

            if (join_worker) {
                fit_worker_.join();
            }

            ApproximatorOutputs outputs{ commands_out, status_out, debug_out };

            if (!curve_store.capture()) {
                outputs.error("missing_curve_or_baseline");
                set_status(Status::error);
                return {};
            }

            set_status(Status::capturing);
            return {};
        }
    };

    message<> fit_message{
        this,
        "fit",
        "Fit EQ parameters to current target curve",
        MIN_FUNCTION {
            bool join_worker = false;
            if (!curve_store.has_captured_curve()) {
                ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
                outputs.error("no_captured_curve");
                set_status(Status::error);
                return {};
            }

            const auto curve = curve_store.captured_residual_curve();

            {
                std::lock_guard<std::mutex> lock(fit_mutex_);
                if (fit_running_) {
                    ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
                    outputs.error("fit_in_progress");
                    return {};
                }

                if (fit_worker_.joinable()) {
                    join_worker = true;
                }

                fit_running_ = true;
                pending_error_.clear();
                pending_result_.reset();
            }

            if (join_worker) {
                fit_worker_.join();
            }

            ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
            set_status(Status::processing);

            fit_worker_ = std::thread([this, curve]() mutable {
                try {
                    const auto result = optimizer.fit(curve);
                    {
                        std::lock_guard<std::mutex> lock(fit_mutex_);
                        pending_result_ = result;
                    }
                }
                catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(fit_mutex_);
                    pending_error_ = e.what();
                }

                {
                    std::lock_guard<std::mutex> lock(fit_mutex_);
                    fit_running_ = false;
                }

                fit_delivery.set();
            });

            return {};
        }
    };

    ~ConsolidatorApproximator() override {
        if (fit_worker_.joinable()) {
            fit_worker_.join();
        }
    }

private:
    void deliver_fit_result() {
        std::optional<EqOptimizer::FitResult> result;
        std::string error;

        {
            std::lock_guard<std::mutex> lock(fit_mutex_);
            result = pending_result_;
            error = pending_error_;
            pending_result_.reset();
            pending_error_.clear();
        }

        if (fit_worker_.joinable()) {
            fit_worker_.join();
        }

        ApproximatorOutputs outputs{ commands_out, status_out, debug_out };

        if (!error.empty()) {
            outputs.error(error.c_str());
            set_status(Status::error);
            return;
        }

        if (result) {
            outputs.loss(result->loss);
            outputs.send_commands(result->params);
            set_status(Status::done);
        }
    }

    void set_status(Status new_status) {
        if (status_ == new_status) {
            return;
        }

        status_ = new_status;

        ApproximatorOutputs outputs{ commands_out, status_out, debug_out };
        switch (new_status) {
            case Status::idle:
                break;
            case Status::capturing:
                outputs.capturing();
                break;
            case Status::processing:
                outputs.processing();
                break;
            case Status::done:
                outputs.done();
                break;
            case Status::error:
                outputs.status_error();
                break;
        }
    }

    ApproximatorCurveStore curve_store;
    EqOptimizer optimizer;
    std::mutex fit_mutex_;
    std::thread fit_worker_;
    bool fit_running_ = false;
    std::optional<EqOptimizer::FitResult> pending_result_;
    std::string pending_error_;
    Status status_ = Status::idle;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
