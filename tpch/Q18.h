// c_name, c_custkey, o_orderkey, o_orderdate, o_totalprice, sum(quant)
using Q18_elt = tuple<char *, uint, uint, Date, uint, double>;
using Q18_rtype = sequence<Q18_elt>;

Q18_rtype Q18(maps m, int q_quantity) {
    using elt = Q18_elt;
    using r_seq = pam_seq<elt>;

    struct Join {
        using T = r_seq;

        static T identity() { return r_seq(); }

        static T add(T a, T b) { return r_seq::join2(move(a), move(b)); }
    };

    auto cust_f = [&](customer_map::E &ce) -> r_seq {
        Customer &c = ce.second.first;
        order_map &omap = ce.second.second;
        auto order_f = [&](order_map::E &oe) -> r_seq {
            ol_map &lmap = oe.second.second;
            Order &o = oe.second.first;
            auto orderline_f = [&](ol_map::E &e) -> double { return e.ol_amount; };
            int v = ol_map::map_reduce(lmap, orderline_f, Add<double>());
            if (v > q_quantity)
                return r_seq(elt(c.c_last(), c.c_id, o.o_id,
                                 o.o_entry_d, o.o_ol_cnt, v));
            else return r_seq();
        };
        return order_map::map_reduce(omap, order_f, Join());
    };

    r_seq result = customer_map::map_reduce(m.cm, cust_f, Join(), 10);
    sequence <elt> r2 = r_seq::entries(result);

    auto less = [](elt a, elt b) {
        return (get<5>(a) > get<5>(b) ||   // decrease on totalprice
                (get<5>(a) == get<5>(b) && // increase on orderdate
                 Date::less(get<3>(a), get<3>(b))));
    };
    return pbbs::sample_sort(r2, less);
}

double Q18time(maps m, bool verbose) {
    timer t;
    t.start();
    int q_quantity = 200;

    Q18_rtype result = Q18(m, q_quantity);
    double ret_tm = t.stop();
    cout << "Q18 : " << ret_tm << endl;

    if (verbose) {
        for (int i = 0; i<10; i++) {
            Q18_elt a = result[i];
            cout << "Q18:" << endl
                 << get<0>(a) << ", " << get<1>(a) << ", " << get<2>(a) << ", "
                 << get<3>(a) << ", " << get<4>(a) << ", " << get<5>(a) << endl;
        }
    }
    return ret_tm;
}

