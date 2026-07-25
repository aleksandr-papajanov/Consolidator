#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace consolidator::optimization {

enum class SearchDirection {
    Minimize,
    Maximize
};

struct BeamSearchOptions final {
    std::size_t beamWidth = 4;
    std::size_t maximumDepth = 6;
    std::size_t maximumChildrenPerState = 12;
    std::size_t maximumFallbackStates = 64;
    std::size_t maximumNonImprovingDepths = 3;
    std::size_t maximumBacktracks = 6;
    std::size_t maximumEvaluatedStates = 1024;
    SearchDirection direction = SearchDirection::Minimize;
};

struct BeamSearchStatistics final {
    std::size_t evaluatedStates = 0;
    std::size_t expandedStates = 0;
    std::size_t completedDepth = 0;
    std::size_t backtracks = 0;
    std::size_t fallbackStates = 0;
    bool budgetExceeded = false;
};

template <typename State>
struct BeamSearchResult final {
    std::optional<State> bestState;
    double bestScore = std::numeric_limits<double>::quiet_NaN();
    bool completed = false;
    bool cancelled = false;
    BeamSearchStatistics statistics;
};

template <typename State>
class BeamSearch final {
public:
    using CandidateGenerator = std::function<std::vector<State>(
        const State& state,
        std::size_t depth
    )>;
    using CandidateEvaluator = std::function<double(const State& state)>;
    using CancellationPredicate = std::function<bool()>;

    explicit BeamSearch(BeamSearchOptions options = {})
        : options(options) {
        ValidateOptions();
    }

    BeamSearchResult<State> Search(
        const State& initialState,
        const CandidateGenerator& generateCandidates,
        const CandidateEvaluator& evaluateCandidate,
        const CancellationPredicate& shouldCancel = {}
    ) const {
        if (!generateCandidates) throw std::invalid_argument("beam_generator_required");
        if (!evaluateCandidate) throw std::invalid_argument("beam_evaluator_required");

        BeamSearchResult<State> result;
        std::vector<Node> beam;
        beam.push_back({
            initialState,
            Evaluate(initialState, evaluateCandidate, result),
            0
        });
        UpdateBest(beam.front(), result);
        std::vector<Node> fallback;
        std::size_t nonImprovingDepths = 0;

        while (!beam.empty()) {
            if (IsCancelled(shouldCancel)) {
                result.cancelled = true;
                return result;
            }

            if (!HasExpandableState(beam)) {
                if (RestoreFallback(fallback, beam, result)) continue;
                result.completed = true;
                return result;
            }

            std::vector<Node> nextBeam;
            for (const auto& node : beam) {
                if (IsCancelled(shouldCancel)) {
                    result.cancelled = true;
                    return result;
                }

                ++result.statistics.expandedStates;
                if (node.depth >= options.maximumDepth) continue;
                auto candidates = generateCandidates(node.value, node.depth);
                const auto candidateCount = std::min(
                    candidates.size(), options.maximumChildrenPerState);
                for (std::size_t index = 0; index < candidateCount; ++index) {
                    if (IsCancelled(shouldCancel)) {
                        result.cancelled = true;
                        return result;
                    }
                    if (result.statistics.evaluatedStates >= options.maximumEvaluatedStates) {
                        result.statistics.budgetExceeded = true;
                        result.completed = true;
                        return result;
                    }

                    auto& candidate = candidates[index];
                    const auto score = Evaluate(candidate, evaluateCandidate, result);
                    if (!std::isfinite(score)) continue;
                    nextBeam.push_back({ std::move(candidate), score, node.depth + 1 });
                }
            }

            if (nextBeam.empty()) {
                if (!RestoreFallback(fallback, beam, result)) {
                    result.completed = true;
                    return result;
                }
                nonImprovingDepths = 0;
                continue;
            }

            SortByScore(nextBeam);
            if (nextBeam.size() > options.beamWidth) {
                fallback.insert(
                    fallback.end(),
                    std::make_move_iterator(nextBeam.begin() + options.beamWidth),
                    std::make_move_iterator(nextBeam.end()));
                nextBeam.resize(options.beamWidth);
                TrimFallback(fallback);
            }

            bool improved = false;
            for (const auto& node : nextBeam) improved = UpdateBest(node, result) || improved;
            beam = std::move(nextBeam);
            for (const auto& node : beam) {
                result.statistics.completedDepth = std::max(
                    result.statistics.completedDepth, node.depth);
            }

            if (improved) {
                nonImprovingDepths = 0;
                continue;
            }

            ++nonImprovingDepths;
            if (nonImprovingDepths >= options.maximumNonImprovingDepths &&
                result.statistics.backtracks < options.maximumBacktracks &&
                RestoreFallback(fallback, beam, result)) {
                nonImprovingDepths = 0;
            }
        }

        result.completed = true;
        return result;
    }

private:
    struct Node final {
        State value;
        double score = 0.0;
        std::size_t depth = 0;
    };

    void ValidateOptions() const {
        if (options.beamWidth == 0) throw std::invalid_argument("beam_width_must_be_positive");
        if (options.maximumChildrenPerState == 0) {
            throw std::invalid_argument("beam_children_limit_must_be_positive");
        }
        if (options.maximumFallbackStates == 0) {
            throw std::invalid_argument("beam_fallback_limit_must_be_positive");
        }
        if (options.maximumNonImprovingDepths == 0) {
            throw std::invalid_argument("beam_stagnation_limit_must_be_positive");
        }
        if (options.maximumEvaluatedStates == 0) {
            throw std::invalid_argument("beam_evaluation_budget_must_be_positive");
        }
    }

    double Evaluate(
        const State& state,
        const CandidateEvaluator& evaluator,
        BeamSearchResult<State>& result
    ) const {
        if (result.statistics.evaluatedStates >= options.maximumEvaluatedStates) {
            result.statistics.budgetExceeded = true;
            return std::numeric_limits<double>::infinity();
        }
        const auto score = evaluator(state);
        ++result.statistics.evaluatedStates;
        return score;
    }

    bool IsBetter(double left, double right) const {
        if (options.direction == SearchDirection::Minimize) return left < right;
        return left > right;
    }

    void SortByScore(std::vector<Node>& nodes) const {
        std::stable_sort(nodes.begin(), nodes.end(),
            [this](const Node& left, const Node& right) {
                return IsBetter(left.score, right.score);
            });
    }

    bool UpdateBest(const Node& node, BeamSearchResult<State>& result) const {
        if (!std::isfinite(node.score)) return false;
        if (!result.bestState || IsBetter(node.score, result.bestScore)) {
            result.bestState = node.value;
            result.bestScore = node.score;
            return true;
        }
        return false;
    }

    bool HasExpandableState(const std::vector<Node>& beam) const {
        return std::any_of(beam.begin(), beam.end(), [this](const Node& node) {
            return node.depth < options.maximumDepth;
        });
    }

    void TrimFallback(std::vector<Node>& fallback) const {
        SortByScore(fallback);
        if (fallback.size() > options.maximumFallbackStates) {
            fallback.resize(options.maximumFallbackStates);
        }
        // Keep this statistic local to the search result through the caller.
    }

    bool RestoreFallback(
        std::vector<Node>& fallback,
        std::vector<Node>& beam,
        BeamSearchResult<State>& result
    ) const {
        if (result.statistics.backtracks >= options.maximumBacktracks) return false;
        fallback.erase(
            std::remove_if(fallback.begin(), fallback.end(), [this](const Node& node) {
                return node.depth >= options.maximumDepth;
            }),
            fallback.end());
        if (fallback.empty()) return false;

        SortByScore(fallback);
        const auto count = std::min(fallback.size(), options.beamWidth);
        beam.clear();
        beam.insert(
            beam.end(),
            std::make_move_iterator(fallback.begin()),
            std::make_move_iterator(fallback.begin() + count));
        fallback.erase(fallback.begin(), fallback.begin() + count);
        ++result.statistics.backtracks;
        result.statistics.fallbackStates = fallback.size();
        return true;
    }

    static bool IsCancelled(const CancellationPredicate& shouldCancel) {
        return shouldCancel && shouldCancel();
    }

    BeamSearchOptions options;
};

} // namespace consolidator::optimization
