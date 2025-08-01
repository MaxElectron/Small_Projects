#include <iostream>
#include <cstdint>

// I'm xorry
template <class T>
struct N;
template <class T>
struct L;

template <class T>
struct N
{
    using U = uintptr_t;
    T d;
    N *b;
    N(T v) : d(v), b(0) {}
    void k(N *f)
    {
        if ((N *)((U)f ^ (U)b))
            ((N *)((U)f ^ (U)b))->k(this);
        delete this;
    }
};

template <class T>
struct L
{
    using N = N<T>;
    using U = uintptr_t;
    N *c, *p;
    L(T v) : c(new N(v)), p(0) {}

    ~L()
    {
        if (!c)
            return;
        if ((N *)((U)p ^ (U)c->b))
            ((N *)((U)p ^ (U)c->b))->k(c);
        if (p)
            p->k(c);
        delete c;
    }

    void r()
    {
        if (!c)
            return;
        p = (N *)((U)p ^ (U)c->b ^ (U)c);
        c = (N *)((U)p ^ (U)c);
        p = (N *)((U)p ^ (U)c);
    }

    void l()
    {
        if (!p)
            return;
        c = (N *)((U)c ^ (U)p->b ^ (U)p);
        p = (N *)((U)p ^ (U)c);
        c = (N *)((U)p ^ (U)c);
    }

    void i(T v)
    {
        if (!c)
            return;
        N *n = new N(v);
        n->b = (N *)((U)c ^ (U)p ^ (U)c->b);
        if ((N *)((U)p ^ (U)c->b))
            ((N *)((U)p ^ (U)c->b))->b = (N *)((U)((N *)((U)p ^ (U)c->b))->b ^ (U)c ^ (U)n);
        c->b = (N *)((U)c->b ^ (U)((U)p ^ (U)c->b) ^ (U)n);
    }

    void rm()
    {
        if (!c)
            return;
        N *t = c;
        if ((N *)((U)p ^ (U)t->b))
            ((N *)((U)p ^ (U)t->b))->b = (N *)((U)((N *)((U)p ^ (U)t->b))->b ^ (U)t ^ (U)p);
        if (p)
        {
            p->b = (N *)((U)p->b ^ (U)t ^ (U)((U)p ^ (U)t->b));
            c = p;
            p = (N *)((U)p->b ^ (U)((U)c ^ (U)t->b));
        }
        else
            c = (N *)((U)p ^ (U)t->b);
        delete t;
    }

    T g()
    {
        return T{} ^ (((c ? c->d : T{}) ^ T{}) & (T) - !!c);
    }
};

int main()
{

}
