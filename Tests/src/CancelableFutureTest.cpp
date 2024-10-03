#include <iostream>

#include <osmscout/async/CancelableFuture.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Call callback immediately") {
  osmscout::CancelableFuture<int>::Promise promise;
  osmscout::CancelableFuture<int> future=promise.Future();

  REQUIRE(future.Value() == std::nullopt);

  int value=0;
  future.OnComplete([&value](const int &computed) {
    value=computed;
  });
  REQUIRE(value == 0);
  promise.SetValue(42);
  REQUIRE(future.Value() == std::make_optional<int>(42));
  REQUIRE(future.StdFuture().get() == 42);
  REQUIRE(value == 42);

  int value2=0;
  future.OnComplete([&value2](const int &computed) {
    value2=computed;
  });
  REQUIRE(value2 == 42);
}

TEST_CASE("Value is idempotent") {
  osmscout::CancelableFuture<int>::Promise promise;
  osmscout::CancelableFuture<int> future=promise.Future();

  REQUIRE(future.Value() == std::nullopt);

  int counter=0;
  int value=0;
  future.OnComplete([&value, &counter](const int &computed) {
    value=computed;
    counter++;
  });
  promise.SetValue(42);
  promise.SetValue(43); // no effect
  REQUIRE(future.Value() == std::make_optional<int>(42));
  REQUIRE(value == 42);
  REQUIRE(counter == 1);
}

TEST_CASE("Cancel is idempotent") {
  osmscout::CancelableFuture<int>::Promise promise;
  osmscout::CancelableFuture<int> future=promise.Future();

  REQUIRE(future.Value() == std::nullopt);

  int counter=0;
  future.OnCancel([&counter]() {
    counter++;
  });
  promise.Cancel();
  promise.Cancel(); // no effect
  future.Cancel(); // no effect
  REQUIRE(future.IsCanceled());
  REQUIRE(promise.IsCanceled());
  REQUIRE(counter == 1);
}
