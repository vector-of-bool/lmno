Why did you make this?
######################

Excellent question.


Beginnings
**********

In October of 2021, I published `a blog post entitled "Stringy Templates"`__,
which outlined the new C++20 features that now afforded developers the ability
to parameterize templates on strings |--| something which used to be very
difficult and un-ergonomic.

Earlier that year, I had also tweeted screenshots of an "embedded scripting
language" I had hacked together during my Summer break:

__ https://vector-of-bool.github.io/2021/10/22/string-templates.html

.. image:: /static/scripting.png
    :align: center
    :alt: Screenshot of an embedded scripting language, different from lmno,
        but also using string literals as template arguments.

The language itself was very ad-hoc and not particularly interesting. It was
more of an excercise in "*is* this possible" rather than "*what* is possible."

At the end of the *Stringy Templates* blog post, I hinted that one could "take
the idea further", and that there would be an eventual follow-up.


Ruminating
**********

This idea sat in the back of my mind for a long time, and yet I wasn't really
sure what I could make from it. The language that I had hacked together was very
"C-like", and it wasn't something that one would find compelling, as anything
you could write in it wasn't significantly different from what one could write
in regular C++.

I wanted an idea for an embeddable language that was *significantly* different
from the existing C++ paradigms. I have a particular love of C++ and Python both
for their flexibility and support of multi-paradigm programming. Being able to
pull out the right tool for the task at hand *without* needing to call out to a
separate API or foreign function is something I appreciate greatly. One might
refer to it as *in-situ paradigm shifting*.


Ideas
*****

I cannot quite recall what about it drew my attention, but the array programming
languages gave me the "lightbulb moment" of discovery. Not specifically for
their array-handling features, but for their *symbolic representation*, and the
terseness of expressing and composing fundamental algorithms.

I enjoy the C++20 :cpp:`std::ranges` library additions. I enjoy the expressivity
and ease with which one can compose small building blocks to build larger
machinery. :cpp:`std::ranges` does come with downsides, however: It can be slow
to compile, difficult to read, and the error messages can be ghastly. I would
like a more compact and terse way to compose the same components. That's a good
candidate for an embedded language.

My interest was also piqued by developments in heterogeneous computing, the
integration story of which is fairly cumbersome. For most of computing, programs
for external compute devices need be written out-of-line and given to a
dedicated toolchain for preparation. Some efforts are underway to allow the
device programs to be written within the C++ sources which use them, but this
still requires language-external toolchain support to intercept the C++ sources
and emit the intermediate representation that will later be dispatched to
compute devices. What if we could use the host C++ compiler itself to generate
the IR from an embedded language *within the very same translation unit*?

.. code-block:: c++

    using code = parse_t<R"(
        ⟨program code here⟩
    )">;
    device.exec<code>(param_a, param_b, param_c);


Experimenting
*************

In late 2022, I began prodding at the idea of another embedded language. This
time, I :strike:`plagarized` *took inspiration from* array languages such as
APL__ and BQN__, opting for a fully-symbolic language. The grammar of APL is
very simple to understand, provided you have some understanding of the
symbology. This started as just an experiment to see "what is possible" with
compile-time parsing.

__ https://aplwiki.com/
__ https://aplwiki.com/wiki/BQN

After poking around and expanding on the idea further and further, I found
myself subject to a kind of *snowball effect*, where I began to take the idea
more seriously and consider that maybe I had something that people might
actually find *useful* rather than it just be a novelty.

And thus, |lmno| was born.
