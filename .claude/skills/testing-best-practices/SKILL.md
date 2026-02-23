---
name: cpp23-testing
description: Best practices for working with the C++23 application's test suite. Uses Boost.Test for unit and integration tests and Google Benchmark for performance tests. Use this skill whenever writing, modifying, running, or debugging tests. Also use when fixing bugs — a regression test MUST accompany every bug fix.
---

# C++23 Test Suite — Claude Code Best Practices

## Core Principle

Every code change that touches application logic must be accompanied by appropriate test coverage. Every bug fix **must** include a regression test that reproduces the original failure before the fix is applied. No exceptions.

---

## Test Categories

This project maintains three test categories. Understand which category applies before writing any test.

### Unit Tests
- Test a single function, class, or module in isolation.
- All external dependencies (network, filesystem, databases, other services) must be mocked or stubbed.
- Must execute in **under 100ms per test case**. If a unit test is slow, it's probably an integration test in disguise.
- Naming convention: `test/unit/<module>/<TestFixtureName>_test.cpp`
- Use descriptive test names that encode the scenario and expected outcome:
  ```cpp
  BOOST_AUTO_TEST_SUITE(WalletBalance)

  BOOST_AUTO_TEST_CASE(ReturnsZero_WhenNewlyCreated, * boost::unit_test::label("unit"))
  {
      Wallet w;
      BOOST_TEST(w.balance() == 0);
  }

  BOOST_AUTO_TEST_CASE(ThrowsOverdraftError_WhenWithdrawalExceedsBalance, * boost::unit_test::label("unit"))
  {
      Wallet w;
      BOOST_CHECK_THROW(w.withdraw(100), OverdraftError);
  }

  BOOST_AUTO_TEST_SUITE_END()
  ```

### Integration Tests
- Test interactions between two or more real components (services, databases, APIs, file systems).
- May use test containers, in-memory databases, or sandboxed network services.
- Naming convention: `test/integration/<feature>/<TestFixtureName>_itest.cpp`
- Each integration test must document which real components it exercises in a comment block at the top of the file.
- Manage setup/teardown carefully — use fixtures (`SetUp`/`TearDown` or RAII wrappers) to guarantee clean state.

### Performance Tests
- Measure latency, throughput, memory usage, or resource consumption against defined baselines.
- Naming convention: `test/perf/<feature>/<TestFixtureName>_perftest.cpp`
- Every performance test must define an **explicit threshold** (e.g., p99 latency < 5ms, throughput > 10k ops/sec).
- Use `benchmark::DoNotOptimize` or equivalent to prevent the compiler from eliding work.
- Performance tests should be tagged/labelled so they can be excluded from normal CI runs and executed on dedicated hardware or in a separate pipeline stage.
- Record results in a machine-readable format (JSON/CSV) for trend tracking.

---

## Regression Tests — Mandatory for Bug Fixes

When fixing a bug, **always** follow this sequence:

1. **Write the regression test first.** Create a test case that reproduces the bug. Confirm it **fails** against the current (unfixed) code. This is non-negotiable — it proves the test actually captures the defect.
2. **Apply the fix.**
3. **Confirm the regression test now passes.**
4. **Place the test** in the appropriate category (usually unit, sometimes integration) alongside related tests.
5. **Include in the test name or a comment** a reference to the bug tracker ID or a brief description of the defect:
   ```cpp
   // Regression: BUG-1042 — Parser crashed on empty input with trailing delimiter
   BOOST_AUTO_TEST_CASE(HandlesEmptyInputWithTrailingDelimiter_Bug1042,
       * boost::unit_test::label("unit")
       * boost::unit_test::label("regression"))
   {
       auto result = CsvParser::parse(",");
       BOOST_TEST(result.has_value());
       BOOST_TEST(result->rows.size() == 0u);
   }
   ```

**Rationale:** Bugs that escape testing once will escape again without a pinned regression test. The cost of writing one is negligible compared to the cost of a repeat incident.

---

## C++23 Testing Standards

### Language & Library Usage
- Prefer C++23 features where they improve clarity or correctness: `std::expected`, `std::print`, `std::ranges`, `constexpr` enhancements, deducing `this`, `std::unreachable`.
- Use `constexpr` tests where feasible — `static_assert` is the ultimate compile-time unit test.
- Prefer `std::expected` over exceptions for functions under test that model recoverable errors; test both the value and error paths.
- Use structured bindings and pattern-matching-style `if` with initializers for readable assertions.

### Test Framework Conventions

**Unit & Integration Tests: Boost.Test**

- Use the single-header or linked variant consistently across the project. Do not mix `BOOST_TEST_MODULE` definitions across translation units in the same test binary.
- Prefer `BOOST_TEST` (the universal macro) over legacy `BOOST_CHECK` / `BOOST_REQUIRE` — it provides richer failure diagnostics with expression decomposition:
  ```cpp
  BOOST_TEST(wallet.balance() == 0);          // prints both sides on failure
  BOOST_TEST(result.has_value());
  BOOST_TEST(vec.size() == 3u);
  ```
- Use `BOOST_REQUIRE` (or `BOOST_TEST` with `boost::test_tools::critical`) only when a failure makes subsequent assertions meaningless (e.g., a null pointer that will be dereferenced).
- Use `BOOST_DATA_TEST_CASE` for data-driven / parameterised tests instead of copy-pasting test bodies:
  ```cpp
  BOOST_DATA_TEST_CASE(ParsesValidInput,
      bdata::make({"42", "0", "-1", "2147483647"}), input)
  {
      auto result = parse_int(input);
      BOOST_TEST(result.has_value());
  }
  ```
- Use `BOOST_AUTO_TEST_SUITE` / `BOOST_AUTO_TEST_SUITE_END` to group related tests into suites that mirror the module structure.
- Use `BOOST_FIXTURE_TEST_CASE` or `BOOST_FIXTURE_TEST_SUITE` for shared setup/teardown. Keep fixtures minimal — if `setup()` is longer than the test, simplify.
- For template-heavy code, use `BOOST_AUTO_TEST_CASE_TEMPLATE` with an `mp_list` or `boost::mpl::list` of types rather than duplicating tests per type.
- Use tolerance-based comparisons for floating-point:
  ```cpp
  BOOST_TEST(result == 3.14, boost::test_tools::tolerance(1e-6));
  ```
- Use `BOOST_TEST_MESSAGE` for diagnostic output that only appears on failure, not `std::cout`.
- Organise test suites with decorators for labelling:
  ```cpp
  BOOST_AUTO_TEST_CASE(MyTest, * boost::unit_test::label("unit"))
  ```

**Performance Tests: Google Benchmark**

- Every benchmark must use `benchmark::DoNotOptimize` and/or `benchmark::ClobberMemory` to prevent the compiler from eliding the work under test.
- Define explicit performance thresholds in comments or in a companion JSON baseline file. The benchmark itself measures; a CI script or wrapper compares against the baseline.
- Use `BENCHMARK_DEFINE_F` with fixtures for benchmarks that need setup (database connections, data loading) to keep setup cost out of the timed region.
- Use range-based benchmarks (`->Range(...)` or `->RangeMultiplier(...)`) to characterise scaling behaviour, not just single-point latency.
- Use `->Iterations(N)` or `->MinTime(X)` to ensure stable results; avoid relying on the default iteration count for microbenchmarks that complete in nanoseconds.
- Register benchmarks with descriptive names using `BENCHMARK(...)->Name("Feature/Scenario/Size")` for readable output and trend tracking.
- Output results to JSON (`--benchmark_out=results.json --benchmark_out_format=json`) for automated regression comparison.

### Mocking
- Use the project's established mocking framework (e.g., trompeloeil, FakeIt, or manual fakes). Boost.Test does not include a mocking library — pick one and use it consistently.
- Prefer dependency injection (constructor or template parameter) over link-time mocking.
- Mock at interface boundaries, not on concrete classes.
- If introducing a mock for the first time on a dependency, place the mock definition in `test/mocks/` for reuse.

---

## Test Quality Checklist

Before submitting any test code, verify:

- [ ] **Deterministic.** No flaky behaviour. No dependence on wall-clock time, random seeds without fixed state, or external services for unit tests.
- [ ] **Independent.** Tests do not depend on execution order. Each test creates and destroys its own state.
- [ ] **Focused.** Each test case validates one logical behaviour. Multiple assertions are fine if they all relate to the same behaviour.
- [ ] **Named clearly.** The test name alone conveys what is being tested, under what conditions, and what the expected outcome is.
- [ ] **Fast.** Unit tests < 100ms. Integration tests < 5s. Performance tests have their own time budget.
- [ ] **No suppressed failures.** Never comment out a failing test or add `DISABLED_` prefix without a tracked issue.
- [ ] **Compiler warnings clean.** Test code is compiled with the same warning flags as production code. `-Werror` applies.

---

## What to Do When Modifying Existing Code

| Scenario | Required Action |
|---|---|
| **Fixing a bug** | Write regression test (see above). Mandatory. |
| **Adding a new feature** | Add unit tests for all public API surfaces. Add integration tests if the feature crosses component boundaries. Add performance tests if the feature is on a latency-sensitive path. |
| **Refactoring** | Existing tests must continue to pass with no modifications to test assertions (only structural changes like renames are acceptable). If tests need assertion changes, that's a behaviour change, not a refactor — treat it as a new feature. |
| **Removing a feature** | Remove corresponding tests. Don't leave orphaned test code. |
| **Changing a dependency** | Re-run integration and performance tests that exercise that dependency. Update mocks if the dependency interface changed. |

---

## Build & Run Conventions

- **Unit tests:** Run on every commit and in every CI pipeline. Must all pass for a merge to proceed.
- **Integration tests:** Run in CI but may be in a separate stage. Gate merges on these as well.
- **Performance tests:** Run on a schedule or manually triggered. Do not gate merges unless a regression exceeds the defined threshold by a significant margin.
- Use CTest with labels to run categories independently. Boost.Test binaries accept `--run_test` and label filters; Google Benchmark binaries accept `--benchmark_filter`:
  ```bash
  # Via CTest labels
  ctest --label-regex unit
  ctest --label-regex integration
  ctest --label-regex perf

  # Direct Boost.Test execution with suite/label filters
  ./unit_tests --run_test=WalletSuite
  ./unit_tests --run_test=@unit           # by label

  # Direct Google Benchmark execution with regex filter
  ./perf_tests --benchmark_filter="Parser/.*"
  ./perf_tests --benchmark_out=results.json --benchmark_out_format=json
  ```
- Use CMake `add_test()` with appropriate labels for each category.

---

## Common Pitfalls — Avoid These

1. **Testing implementation details.** Test observable behaviour, not internal state. If a refactor breaks your test but not the feature, the test was too tightly coupled.
2. **Enormous fixtures.** If `SetUp()` is longer than the test body, break the fixture apart or simplify the system under test.
3. **Ignoring edge cases.** Explicitly test: empty inputs, maximum-size inputs, nullopt/unexpected values, boundary values, concurrent access (if applicable).
4. **Copy-paste test bodies.** Use parameterised tests or test generators instead of duplicating test logic with minor variations.
5. **Missing negative tests.** Don't only test the happy path. Test that the code correctly rejects invalid input, handles errors, and throws/returns errors as documented.
6. **Floating-point equality.** Never use exact equality for floating-point comparisons. Use `BOOST_TEST(a == b, boost::test_tools::tolerance(epsilon))` or compare with an explicit absolute/relative tolerance.
7. **Leaking state between tests.** Global or static mutable state is the leading cause of flaky tests. Avoid it, or reset it explicitly in fixtures.
