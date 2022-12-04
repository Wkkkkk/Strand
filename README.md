# Strand

#### Description
This is an implement for Boost::Asio::Strand. To visually demonstrate what happens with the IO threads and handlers, [Remotery](https://github.com/Celtoys/Remotery) is used.

The code used emulates 4 worker threads, and 8 connections. Handlers (aka *work items*) with a random workload of `[5ms,15ms]`for a random connection are placed in the worker queue.

Without strand

![time-slice](/imgs/time-slice.png)

With strand

![time-slice](/imgs/time-slice-strand.png)
