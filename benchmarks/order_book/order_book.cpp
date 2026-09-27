#include "lab/benchmark.hpp"
#include "lab/order_book.hpp"
#include "lab/order_book_perf.hpp"
#include "lab/order_book_reference.hpp"
#include "lab/order_book_workload.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>

namespace lab {
namespace {
class Replay {
    const book::Trace& trace_;
public:
    std::unique_ptr<book::MapBook> book;
    std::vector<book::Outcome> outcomes;
    std::vector<book::Trade> trades;
    std::size_t filled = 0;
    explicit Replay(const book::Trace& trace) : trace_(trace), outcomes(trace.events.size()),
        trades(trace.trades.size() + trace.limits.max_orders) {}
    void reset() {
        book = std::make_unique<book::MapBook>(trace_.limits);
        filled = 0;
        for(const auto& e : trace_.initial) {
            const auto result = book->apply(e, trades);
            if(result.status != book::Status::ok || result.trade_count) throw std::runtime_error("order book setup mismatch");
        }
    }
    void event(std::size_t i) {
        outcomes[i] = book->apply(trace_.events[i], std::span(trades).subspan(filled));
        filled += outcomes[i].trade_count;
    }
    void validate(std::size_t count) {
        if(!std::equal(outcomes.begin(), outcomes.begin() + count, trace_.outcomes.begin()))
            throw std::runtime_error("order book outcomes differ from reference");
        std::size_t expected_trades = 0;
        for(std::size_t i = 0; i < count; ++i) expected_trades += trace_.outcomes[i].trade_count;
        if(filled != expected_trades || !std::equal(trades.begin(), trades.begin() + filled, trace_.trades.begin()))
            throw std::runtime_error("order book trade sequence differs from reference");
        auto expected_state = trace_.final_state;
        if(count != trace_.events.size()) {
            book::ReferenceBook prefix(trace_.limits);
            std::vector<book::Trade> scratch(trace_.limits.max_orders);
            for(const auto& e : trace_.initial) prefix.apply(e, scratch);
            for(std::size_t i = 0; i < count; ++i) prefix.apply(trace_.events[i], scratch);
            expected_state = prefix.snapshot();
        }
        if(book->snapshot() != expected_state || !book->invariant())
            throw std::runtime_error("order book final state/invariant mismatch");
    }
};
void annotate(Result& result, const book::Trace& trace, std::size_t prefix, std::size_t passes) {
    std::array<std::size_t, 4> kinds{};
    std::size_t accepted = 0, missing = 0, rejected = 0, trades = 0;
    std::uint64_t matched = 0, unfilled_market = 0;
    for(std::size_t i = 0; i < prefix; ++i) {
        ++kinds[static_cast<unsigned>(trace.events[i].kind)];
        const auto status = trace.outcomes[i].status;
        if(status == book::Status::ok) ++accepted;
        else if(status == book::Status::not_found) ++missing;
        else ++rejected;
        trades += trace.outcomes[i].trade_count;
        if(trace.events[i].kind == book::Kind::match) unfilled_market += trace.outcomes[i].remaining;
    }
    for(std::size_t i = 0; i < trades; ++i) matched += trace.trades[i].quantity;
    auto& m = result.metrics;
    m["workload_version"] = 1;
    m["trace_hash_hi"] = static_cast<std::uint32_t>(trace.fingerprint >> 32);
    m["trace_hash_lo"] = static_cast<std::uint32_t>(trace.fingerprint);
    m["initial_orders"] = trace.initial.size(); m["capacity"] = trace.limits.max_orders;
    m["price_min"] = trace.limits.min_price; m["price_max"] = trace.limits.max_price;
    m["trace_events"] = trace.events.size(); m["replayed_events"] = prefix * passes;
    m["add_events"] = kinds[0] * passes; m["cancel_events"] = kinds[1] * passes;
    m["modify_events"] = kinds[2] * passes; m["match_events"] = kinds[3] * passes;
    m["accepted_events"] = accepted * passes; m["not_found_events"] = missing * passes;
    m["rejected_events"] = rejected * passes; m["trades"] = trades * passes;
    m["matched_quantity"] = static_cast<double>(matched * passes);
    m["unfilled_market_quantity"] = static_cast<double>(unfilled_market * passes);
    if(prefix == trace.events.size()) {
        m["peak_orders_per_replay"] = trace.peak_orders;
        m["minimum_orders_per_replay"] = trace.minimum_orders;
        m["final_orders_per_replay"] = trace.final_state.size();
    }
    result.notes += " map_list baseline: map price levels + list FIFO + unordered_map ID->stable iterators; default allocation and hash growth remain timed. Synthetic 60/25/10/5 exact per full 100-event block; not exchange statistics. price ticks 1..20000. size=initial orders, NOT bytes (legacy JSON config key size_bytes). Hash identifies full initial+event stream. Setup/generation/reset/validation/warmup excluded. Closed-loop replay; no queueing/network latency. All outcomes/trades/final state checked against independent vector oracle outside timing. Baseline stages Add/priority-changing Modify nodes before matching for allocation-failure rollback; even fully crossing Add allocates a node. ";
}
} // namespace

Results order_book(const Config& c) {
    require_threads(c, 1);
    if(!selected(c, "map_list")) return {};
    const bool latency = c.benchmark == "order_book_latency";
    if(c.warmup > 100000) throw std::invalid_argument("order book warmup <=100000 required");
    const auto trace = book::make_trace(c.size, latency ? c.iterations : c.batch, c.seed);
    Replay replay(trace);
    Result result;
    result.benchmark = c.benchmark; result.variant = "map_list";
    if(latency) {
        // Warm up using the same trace, but restore initial state afterwards.
        std::size_t remaining = c.warmup;
        while(remaining) {
            replay.reset();
            const auto count = std::min(remaining, trace.events.size());
            for(std::size_t i = 0; i < count; ++i) replay.event(i);
            replay.validate(count); remaining -= count;
        }
        replay.reset();
        result.mode = "latency"; result.unit = "ns/event"; result.sample_kind = "single_event";
        result.samples.reserve(c.iterations);
        const auto begin = Clock::now();
        for(std::size_t i = 0; i < trace.events.size(); ++i) {
            auto output = std::span(replay.trades).subspan(replay.filled);
            compiler_barrier(); const auto a = Clock::now();
            const auto outcome = replay.book->apply(trace.events[i], output);
            do_not_optimize(outcome); const auto b = Clock::now();
            replay.outcomes[i] = outcome; replay.filled += outcome.trade_count;
            const double ns = elapsed_ns(a, b);
            result.samples.push_back(ns); result.total_ns += ns; ++result.total_operations;
            if(c.duration > 0 && std::chrono::duration<double>(b - begin).count() >= c.duration) break;
        }
        replay.validate(result.samples.size());
        annotate(result, trace, result.samples.size(), 1);
        result.notes += "One timer bracket/event; includes apply and trade writes plus timer overhead; sample bookkeeping excluded. --batch unused; --warmup is discarded event count. No per-event PMU. Small samples cannot establish p99.9.";
    } else {
        for(std::size_t i = 0; i < c.warmup; ++i) {
            replay.reset(); for(std::size_t j = 0; j < trace.events.size(); ++j) replay.event(j);
            replay.validate(trace.events.size());
        }
        std::unique_ptr<book::ReplayCounters> counters;
        std::string pmu_reason;
        try { counters = std::make_unique<book::ReplayCounters>(); }
        catch(const std::runtime_error& error) { pmu_reason = error.what(); }
        double cycles = 0, instructions = 0, enabled = 0, running = 0;
        Config measured = c; measured.warmup = 0;
        result = measure(measured, c.benchmark, "map_list", trace.events.size(), [&] {
            for(std::size_t i = 0; i < trace.events.size(); ++i) replay.event(i);
            do_not_optimize(replay.filled);
        }, "throughput", "replay_mean", [&] {
            replay.reset();
            if(counters) try { counters->begin(); }
            catch(const std::runtime_error& error) { pmu_reason = error.what(); counters.reset(); }
        }, [&] {
            if(counters) try {
                const auto counts = counters->end();
                cycles += counts.cycles; instructions += counts.instructions;
                enabled += counts.enabled; running += counts.running;
            } catch(const std::runtime_error& error) { pmu_reason = error.what(); counters.reset(); }
            replay.validate(trace.events.size());
        });
        result.unit = "ns/event";
        annotate(result, trace, trace.events.size(), result.samples.size());
        result.metrics["pmu_measured"] = counters ? 1 : 0;
        if(counters) {
            result.metrics["cycles_raw"] = cycles; result.metrics["instructions_raw"] = instructions;
            result.metrics["time_enabled_ns"] = enabled; result.metrics["time_running_ns"] = running;
            result.metrics["pmu_running_ratio"] = running / enabled;
            if(enabled == running) {
                result.metrics["cycles_per_event"] = cycles / result.total_operations;
                result.metrics["instructions_per_event"] = instructions / result.total_operations;
                if(cycles > 0) result.metrics["ipc"] = instructions / cycles;
            } else if(cycles > 0) result.metrics["scheduled_count_ipc"] = instructions / cycles;
            result.notes += "PMU: calling-thread user-space replay interval only, includes timer/control overhead, excludes reset/validation/warmup; per-event counters omitted when multiplexed.";
        } else result.notes += "PMU NOT MEASURED: " + pmu_reason + ".";
        result.notes += " --batch=events/replay, iterations=replay samples, warmup=discarded full replays. Timed throughput includes event loop/outcome journal writes. Replay mean p99 is NOT individual event p99.";
    }
    return {std::move(result)};
}
} // namespace lab
