#pragma once

#include "DeviceOptimizer.h"
#include "FitAudioBuffer.h"
#include "Optimization/BeamSearch.h"
#include "Settings/FitOptions.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

struct ApproximatorBeamSearchOptions final {
    consolidator::optimization::BeamSearchOptions searchOptions;
    int structureMaximumEvaluations = 8;
    int finalMaximumEvaluations = consolidator::settings::FitOptions::MaximumEvaluations;
    std::size_t maximumStructuralEvaluations = 32;
    std::size_t maximumTotalEvaluations = 256;
};

class ApproximatorBeamSearchAdapter final {
public:
    using State = consolidator::domain::DspSnapshot;
    using SearchResult = consolidator::optimization::BeamSearchResult<State>;

    struct Result final {
        SearchResult structure;
        std::optional<State> optimizedState;
        double loss = std::numeric_limits<double>::infinity();
    };

    explicit ApproximatorBeamSearchAdapter(ApproximatorBeamSearchOptions options = {})
        : options(options) {
        if (options.structureMaximumEvaluations <= 0 ||
            options.finalMaximumEvaluations <= 0 ||
            options.maximumStructuralEvaluations == 0 ||
            options.maximumTotalEvaluations == 0) {
            throw std::invalid_argument("invalid_beam_optimizer_budget");
        }
    }

    Result Search(
        const State& initialState,
        const FitAudioBuffer& audio,
        const std::atomic<bool>& cancelRequested
    ) {
        decisions = BuildDecisions(initialState);
        if (decisions.empty()) throw std::runtime_error("no_structural_decisions");

        this->audio = &audio;
        this->cancelRequested = &cancelRequested;
        bestOptimizedState.reset();
        bestLoss = std::numeric_limits<double>::infinity();
        totalEvaluationBudget.store(
            options.maximumTotalEvaluations, std::memory_order_release);

        auto searchOptions = options.searchOptions;
        searchOptions.maximumDepth = std::min(
            searchOptions.maximumDepth, decisions.size());
        searchOptions.maximumChildrenPerState = std::max<std::size_t>(
            searchOptions.maximumChildrenPerState, 2);
        searchOptions.maximumEvaluatedStates = std::min(
            searchOptions.maximumEvaluatedStates,
            options.maximumStructuralEvaluations);

        consolidator::optimization::BeamSearch<State> search(searchOptions);
        auto structure = search.Search(
            initialState,
            [this](const State& state, std::size_t depth) {
                return GenerateStructuralCandidates(state, depth);
            },
            [this](const State& state) {
                return EvaluateStructure(state);
            },
            [this]() {
                return this->cancelRequested->load(std::memory_order_acquire);
            });

        if (!structure.cancelled && bestOptimizedState &&
            totalEvaluationBudget.load(std::memory_order_acquire) > 0) {
            DeviceOptimizer optimizer;
            const auto final = optimizer.Optimize(
                *bestOptimizedState,
                *this->audio,
                *this->cancelRequested,
                options.finalMaximumEvaluations,
                &totalEvaluationBudget);
            bestOptimizedState = final.snapshot;
            bestLoss = final.loss;
            structure.bestState = final.snapshot;
            structure.bestScore = final.loss;
        }

        return { std::move(structure), std::move(bestOptimizedState), bestLoss };
    }

private:
    enum class DecisionKind {
        FilterBypass,
    };

    struct StructuralDecision final {
        DecisionKind kind = DecisionKind::FilterBypass;
        std::size_t filterIndex = 0;
    };

    std::vector<StructuralDecision> BuildDecisions(const State& state) const {
        std::vector<StructuralDecision> result;
        const auto* bank = state.eq.SelectedBank();
        if (!bank) return result;

        for (std::size_t index = 0; index < bank->filters.size(); ++index) {
            result.push_back({ DecisionKind::FilterBypass, index });
        }
        return result;
    }

    std::vector<State> GenerateStructuralCandidates(
        const State& state,
        std::size_t depth
    ) const {
        if (depth >= decisions.size()) return {};
        const auto& decision = decisions[depth];
        std::vector<State> candidates;
        candidates.reserve(2);
        for (const bool bypass : { false, true }) {
            auto candidate = state;
            ApplyDecision(candidate, decision, bypass);
            candidates.push_back(std::move(candidate));
        }
        return candidates;
    }

    static void ApplyDecision(
        State& state,
        const StructuralDecision& decision,
        bool bypass
    ) {
        auto* bank = state.eq.SelectedBank();
        if (!bank) return;
        switch (decision.kind) {
            case DecisionKind::FilterBypass:
                bank->filters[decision.filterIndex].bypass = bypass;
                break;
            default:
                break;
        }
    }

    double EvaluateStructure(const State& structure) {
        if (cancelRequested->load(std::memory_order_acquire)) {
            return std::numeric_limits<double>::infinity();
        }

        if (totalEvaluationBudget.load(std::memory_order_acquire) == 0) {
            return std::numeric_limits<double>::infinity();
        }

        DeviceOptimizer optimizer;
        const auto optimized = optimizer.Optimize(
            structure,
            *audio,
            *cancelRequested,
            options.structureMaximumEvaluations,
            &totalEvaluationBudget);
        if (optimized.loss < bestLoss) {
            bestLoss = optimized.loss;
            bestOptimizedState = optimized.snapshot;
        }
        return optimized.loss;
    }

    ApproximatorBeamSearchOptions options;
    std::vector<StructuralDecision> decisions;
    const FitAudioBuffer* audio = nullptr;
    const std::atomic<bool>* cancelRequested = nullptr;
    std::optional<State> bestOptimizedState;
    double bestLoss = std::numeric_limits<double>::infinity();
    std::atomic<std::size_t> totalEvaluationBudget{ 0 };
};
