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
      : myObject(std::forward<Object>(object)) { }

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
  TypeErased<StrategyA> te1((Object1()));
  Object1 o2;
  TypeErased<StrategyB> te2(o2);
  te1.f2(1);
  te2.f1(2);

  std::vector<TypeErased<StrategyA>> objects;
  objects.emplace_back(Object1());
  objects.emplace_back(Object2());
}
```

Although this represent lots of code, modern compilers *should* compile away most of it.

---

## Runtime Type Erasure

One may also want to make ***Type Erasure*** do-able at runtime as follows:

```c++
int main()
{
  std::vector<RuntimeTypeErased> v;
  v.emplace_back(Object1(), StrategyA());
  v.emplace_back(Object2(), StrategyB());
  v.emplace_back(Object1(), StrategyB());
  v.emplace_back(Object2(), StrategyA());
  for (auto&& e : v) {
    e.f1(3);
    e.f2(4);
  }
}
```

This can be achieved with the following, making both *Strategy* Mixins instantiable (by making their methods public):

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

  template<typename MyObject, typename Strategy>
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
  template<typename Object, typename Strategy>
  RuntimeTypeErased(Object&& object, Strategy&& strategy)
  {
    pimpl = std::make_unique<Implementation<std::decay_t<Object>, std::decay_t<Strategy>>>
            (std::forward<Object>(object));
  }

  // Strategy. ONLY this is exposed.
  void f1(int x) { pimpl->ff1(x); }
  void f2(int x) { pimpl->ff2(x); }
};
```

---

## Compound Strategies to Existing Objects

The main drawback with *Type Erasure*'s two interpretations above is that the original type-erased functionality is lost forever and that the original objects can only be copied/moved from. A more general solution would be to be able to compound new Strategies to existing objects as well, and to retain full access to said existing objects:

```c++
int main()
{
  StrategyA sa;
  StrategyB sb;
  auto o1{ std::make_shared<Object1>() };
  auto o2{ std::make_shared<Object2>() };

  std::vector<Compounded> v;
  v.emplace_back(Object1(), StrategyA());
  v.emplace_back(Object2(), StrategyB());
  v.emplace_back(Object1(), StrategyB());
  v.emplace_back(Object2(), StrategyA());
  v.emplace_back(Object1(), sa);
  v.emplace_back(Object2(), sb);
  v.emplace_back(o1, StrategyB());
  v.emplace_back(o2, StrategyA());
  v.emplace_back(o1, sa);
  v.emplace_back(o2, sb);
  v.emplace_back(o1, sb);
  v.emplace_back(o2, sa);
  for (auto&& e : v) {
    e.f1(3);
    e.f2(4);
  }
  o1->a(5);
  o2->c(6);
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

  template<typename MyObject, typename Strategy>
  struct Implementation : public Interface, public Strategy
  {
    std::shared_ptr<MyObject> myObjectPointer;

    template<typename ObjectPointer>
    Implementation(ObjectPointer&& objectPointer)
      : myObjectPointer(std::forward<ObjectPointer>(objectPointer))
    { }

    // Implemented interface.
    // 'this->' needed here to access templated superclass Strategy.
    void ff1(int x) override { this->f1(*myObjectPointer, x); }
    void ff2(int x) override { this->f2(*myObjectPointer, x); }
  };

  std::unique_ptr<Interface> pimpl;

  // Used by the constructor below.
  template<typename Type> struct is_shared_ptr : std::false_type {};
  template<typename Type> struct is_shared_ptr<std::shared_ptr<Type>> : std::true_type {};

public:
  template<typename Object, typename Strategy>
  Compounded(Object&& object, Strategy&& strategy)
  {
    // object is a shared_ptr so pass it as is to make_unique.
    if constexpr (is_shared_ptr<std::decay_t<Object>>::value) {
      pimpl = std::make_unique<Implementation
              <typename std::decay_t<Object>::element_type, std::decay_t<Strategy>>>
              (std::forward<Object>(object));
    // object is NOT a shared_ptr so make a shared_ptr of it and pass it up.
    } else {
      pimpl = std::make_unique<Implementation
              <std::decay_t<Object>, std::decay_t<Strategy>>>
              (std::make_shared<std::decay_t<Object>>(std::forward<Object>(object)));
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