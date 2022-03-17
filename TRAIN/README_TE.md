# C++ Type Erasure

***Type Erasure*** in C++ is a powerful pattern used to readily add new functionality to a set of unrelated already existing classes, without having to re-open or edit said classes. These classes are encapsulated and their original types can be considered *erased* as only the newly defined behaviour remains accessible. This pattern also handles the lifetime of the underlying type-erased objects.

---

## "Standard" Type Erasure

The following was inspired by [Klaus Iglberger](https://github.com/igl42)'s interpretation. One way to implement such a pattern is to use *Mixins* to define this new functionality as a *Strategy*, instead of using free floating functions. To do so:

#### 1. Gather a number of unrelated classes, e.g.:

```c++
class Object1
{
  ...
public:
  void a(int x) { ... }
  void b(int x) { ... }
};
class Object2
{
  ...
public:
  void c(int x) { ... }
  void d(int x) { ... }
};
```

#### 2. Define a set of functions in a Mixin to implement a new Strategy for each class selected above. Optionally, define the same set of functions in more than one such Mixin to implement different Strategies, e.g.:

```c++
class StrategyA
{
protected:
  void f1(Object1& o, int x) { o.a(x); }
  void f2(Object1& o, int x) { o.a(x); o.b(x); }

  void f1(Object2& o, int x) { o.d(x); o.c(x); }
  void f2(Object2& o, int x) { o.d(x); }
};
class StrategyB
{
protected:
  void f1(Object1& o, int x) { o.b(x); }
  void f2(Object1& o, int x) { o.a(x); }

  void f1(Object2& o, int x) { o.d(x); }
  void f2(Object2& o, int x) { o.c(x); }
};
```

#### 3. Copy and paste the following ***Type Erasure*** idiom with the following modifications:

First, for each function name above, determine a corresponding helper function name, e.g.: `ff1` for function `f1()`, `ff2` for function `f2()`, etc.

Then essentially:

1. In the interface, declare each helper function as `virtual pure`.
1. In the templated implementation, define each helper function to call its counterpart function.
1. In the class definition, define functions to call each its counterpart helper function.

More specifically, for each pair `<function>` and `<helper>`:

1. In `struct Interface` right under `// Virtual interface.`, declare: `virtual <return type> <helper>(...) = 0`.
1. In `struct Implementation`, right under `// Implemented interface.`, define: `<return type> <helper>(...) override { this-><function>(myObject, ...); }`.
1. At the bottom of the class definition, right under `// Strategy.`, define: `<return type> <function>(...) { pimpl-><helper>(...); }`.

Resulting in:

```c++
template<typename Strategy>
class TypeErased
{
private:
  struct Interface
  {
    // Virtual default destructor.
    virtual ~Interface() = default;

    // Virtual interface.
    virtual void ff1(int x) = 0;
    virtual void ff2(int x) = 0;
  };

  template<typename MyObject>
  struct Implementation : public Interface, public Strategy
  {
    MyObject myObject;

    template<typename Object>
    Implementation(Object&& object)
      : myObject(std::forward<Object>(object))
    { }

    // Implemented interface.
    // 'this->' needed here to access templated superclass Strategy.
    void ff1(int x) override { this->f1(myObject, x); }
    void ff2(int x) override { this->f2(myObject, x); }
  };

  std::unique_ptr<Interface> pimpl;

public:
  template<typename Object>
  TypeErased(Object&& object)
    : pimpl(std::make_unique<Implementation<std::decay_t<Object>>>
            (std::forward<Object>(object)))
  { }

  // Strategy. ONLY this is exposed.
  void f1(int x) { pimpl->ff1(x); }
  void f2(int x) { pimpl->ff2(x); }
};
```

So this can be used as follows:

```c++
int main()
{
  TypeErased<StrategyA> te1((Object1(1)));
  Object1 o(2);
  TypeErased<StrategyB> te2(o);
  te1.f2(11);
  te2.f1(12);

  std::vector<TypeErased<StrategyA>> objects;
  objects.emplace_back(Object1(3));
  objects.emplace_back(Object2(4));
}
```

Although this represent lots of code, modern compilers *should* compile away most of it.

---

## Runtime Type Erasure

One may also want to make ***Type Erasure*** do-able at runtime as follows:

```c++
int main()
{
  StrategyA sa{ StrategyA() };
  StrategyB sb{ StrategyB() };
  Object1 o1{ Object1(1) };
  Object1 o2{ Object1(2) };

  std::vector<RuntimeTypeErased> v;
  v.emplace_back(o1, sb);
  v.emplace_back(o2, StrategyA());
  v.emplace_back(Object1(1), sa);
  v.emplace_back(Object2(2), StrategyB());
  for (auto&& e : v) {
    e.f1(11);
    e.f2(12);
  }
}
```

This can be achieved with the following, making both *Strategy* Mixins instantiable classes (by making their methods public), as well as ensuring they have valid default, copy and move constructors:

```c++
class RuntimeTypeErased
{
private:
  struct Interface
  {
    // Virtual default destructor.
    virtual ~Interface() = default;

    // Virtual interface.
    virtual void ff1(int x) = 0;
    virtual void ff2(int x) = 0;
  };

  template<typename MyObject, typename MyStrategy>
  struct Implementation : public Interface
  {
    MyObject myObject;
    MyStrategy myStrategy;

    template<typename Object, typename Strategy>
    Implementation(Object&& object, Strategy&& strategy)
      : myObject(std::forward<Object>(object)),
        myStrategy(std::forward<Strategy>(strategy))
    { }

    // Implemented interface.
    void ff1(int x) override { myStrategy.f1(myObject, x); }
    void ff2(int x) override { myStrategy.f2(myObject, x); }
  };

  std::unique_ptr<Interface> pimpl;

public:
  template<typename Object, typename Strategy>
  RuntimeTypeErased(Object&& object, Strategy&& strategy)
    : pimpl(std::make_unique<Implementation
            <std::decay_t<Object>, std::decay_t<Strategy>>>
            (std::forward<Object>(object), std::forward<Strategy>(strategy)))
  { }

  // Strategy. ONLY this is exposed.
  void f1(int x) { pimpl->ff1(x); }
  void f2(int x) { pimpl->ff2(x); }
};
```

---

## Compound Strategies to Existing Objects

The main drawback with *Type Erasure*'s two interpretations above is that the original type-erased functionality is lost forever and that the original objects can only be copied/moved from. A more general solution would be to be able to compound new Strategies to existing objects as well and to retain full access to said objects:

```c++
int main()
{
  StrategyA s{ StrategyA() };
  auto o{ std::make_shared<Object1>(1) };

  std::vector<Compounded> v;
  v.emplace_back(Object2(2), StrategyB());
  v.emplace_back(Object1(3), s);
  v.emplace_back(o, StrategyA());
  v.emplace_back(o, s);
  for (auto&& e : v) {
    e.f1(11);
    e.f2(12);
  }
  o->a(13);
}
```

Templated class `Compounded` below gets a constructor that detects if the passed object is a `std::shared_ptr` and stores it in the pimpl accordingly.

```c++
class Compounded
{
private:
  struct Interface
  {
    // Virtual default destructor.
    virtual ~Interface() = default;

    // Virtual interface.
    virtual void ff1(int x) = 0;
    virtual void ff2(int x) = 0;
  };

  template<typename Object, typename MyStrategy>
  struct Implementation : public Interface
  {
    std::shared_ptr<Object> myObjectPointer;
    MyStrategy myStrategy;

    template<typename ObjectPointer, typename Strategy>
    Implementation(ObjectPointer&& objectPointer, Strategy&& strategy)
      : myObjectPointer(std::forward<ObjectPointer>(objectPointer)),
        myStrategy(std::forward<Strategy>(strategy))
    { }

    // Implemented interface.
    void ff1(int x) override { myStrategy.f1(*myObjectPointer, x); }
    void ff2(int x) override { myStrategy.f2(*myObjectPointer, x); }
  };

  std::unique_ptr<Interface> pimpl;

  // Used by the constructor below.
  template<typename Type> struct is_shared_ptr : std::false_type {};
  template<typename Type> struct is_shared_ptr<std::shared_ptr<Type>>
    : std::true_type {};

public:
  template<typename Object, typename Strategy>
  Compounded(Object&& object, Strategy&& strategy)
  {
    // object is a shared_ptr so pass it as is to make_unique.
    if constexpr (is_shared_ptr<std::decay_t<Object>>::value) {
      pimpl = std::make_unique<Implementation
              <typename std::decay_t<Object>::element_type, std::decay_t<Strategy>>>
              (std::forward<Object>(object), std::forward<Strategy>(strategy));
    // object is NOT a shared_ptr so make a shared_ptr of it and pass it up.
    } else {
      pimpl = std::make_unique<Implementation
              <std::decay_t<Object>, std::decay_t<Strategy>>>
              (std::make_shared<std::decay_t<Object>> (std::forward<Object>(object)),
                std::forward<Strategy>(strategy));
    }
  }

  // Strategy. ONLY this is exposed.
  void f1(int x) { pimpl->ff1(x); }
  void f2(int x) { pimpl->ff2(x); }
};
```

I can see two drawbacks to this approach:

1. The type-erased object can **not** itself be a `std::shared_ptr`.
1. The type-erased object is stored in a `std::shared_ptr` and not directly, adding a second level of indirection, thus slightly hurting performance.

The main advantage would be that no `clone()` method is needed as compounded objects can be created at will from the original objects.

---

## Compound Stateful Strategies to Existing Objects

What if one wants to compound to a stateful Strategy and keep full access to it as follows:

```c++
int main()
{
  auto s{ std::make_shared<StrategyB>(21) };
  auto o{ std::make_shared<Object1>(1) };

  std::vector<StatefulCompounded> v;
  v.emplace_back(o, s);
  v.emplace_back(o, StrategyA(22));
  v.emplace_back(Object2(2), s);
  v.emplace_back(Object1(3), StrategyB(23));
  o->a(11);
  for (auto&& e : v) {
    e.f1(12);
    e.f2(13);
  }
  s->g();
}
```

Templated class `StatefulCompounded`'s constructor detects also if the passed Strategy is a `std::shared_ptr` and stores it accordingly.

```c++
class StatefulCompounded
{
private:
  struct Interface
  {
    // Virtual default destructor.
    virtual ~Interface() = default;

    // Virtual interface.
    virtual void ff1(int x) = 0;
    virtual void ff2(int x) = 0;
  };

  template<typename Object, typename Strategy>
  struct Implementation : public Interface
  {
    std::shared_ptr<Object> myObjectPointer;
    std::shared_ptr<Strategy> myStrategyPointer;

    template<typename ObjectPointer, typename StrategyPointer>
    Implementation(ObjectPointer&& objectPointer, StrategyPointer&& strategyPointer)
      : myObjectPointer(std::forward<ObjectPointer>(objectPointer)),
        myStrategyPointer(std::forward<StrategyPointer>(strategyPointer))
    { }

    // Implemented interface.
    void ff1(int x) override { myStrategyPointer->f1(*myObjectPointer, x); }
    void ff2(int x) override { myStrategyPointer->f2(*myObjectPointer, x); }
  };

  std::unique_ptr<Interface> pimpl;

  // Used by the constructor below.
  template<typename Type> struct is_shared_ptr : std::false_type {};
  template<typename Type> struct is_shared_ptr<std::shared_ptr<Type>> : std::true_type {};

public:
  template<typename Object, typename Strategy>
  StatefulCompounded(Object&& object, Strategy&& strategy)
  {
    // object is a shared_ptr so pass it as is to make_unique.
    if constexpr (is_shared_ptr<std::decay_t<Object>>::value) {
      // strategy is a shared_ptr so pass it as is to make_unique.
      if constexpr (is_shared_ptr<std::decay_t<Strategy>>::value)
        pimpl = std::make_unique<Implementation<typename std::decay_t<Object>::element_type,
                                                typename std::decay_t<Strategy>::element_type>>
                (std::forward<Object>(object),
                 std::forward<Strategy>(strategy));
      // strategy is NOT a shared_ptr so make a shared_ptr of it and pass it up.
      else
        pimpl = std::make_unique<Implementation<typename std::decay_t<Object>::element_type,
                                                typename std::decay_t<Strategy>>>
                (std::forward<Object>(object),
                 std::make_shared<std::decay_t<Strategy>>(std::forward<Strategy>(strategy)));
    // object is NOT a shared_ptr so make a shared_ptr of it and pass it up.
    } else {
      // strategy is a shared_ptr so pass it as is to make_unique.
      if constexpr (is_shared_ptr<std::decay_t<Strategy>>::value)
        pimpl = std::make_unique<Implementation<typename std::decay_t<Object>,
                                                typename std::decay_t<Strategy>::element_type>>
                (std::make_shared<std::decay_t<Object>>(std::forward<Object>(object)),
                 std::forward<Strategy>(strategy));
      // strategy is NOT a shared_ptr so make a shared_ptr of it and pass it up.
      else
        pimpl = std::make_unique<Implementation<typename std::decay_t<Object>,
                                                typename std::decay_t<Strategy>>>
                (std::make_shared<std::decay_t<Object>>(std::forward<Object>(object)),
                 std::make_shared<std::decay_t<Strategy>>(std::forward<Strategy>(strategy)));
    }
  }

  // Strategy. ONLY this is exposed.
  void f1(int x) { pimpl->ff1(x); }
  void f2(int x) { pimpl->ff2(x); }
};
```

The main disadvantage of this solution is that it introduces a third level of indirection. Nonetheless, it looks like the most general and flexible solution to keep access to both objects and Strategies.

---