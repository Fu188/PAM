using Q14_rtype = double;

double Q14(maps m, const char *start, const char *end) {
    const char promo[] = "PR";
    int plen = strlen(promo);

    delivery_map delivery_range = delivery_map::range(m.dm, Date(start), Date(end));

    using sum = pair<float, float>;
    using Add = Add_Pair <Add<float>, Add<float>>;

    auto date_f = [&](delivery_map::E &e) -> sum {
        auto ol_f = [&](ol_map::E &ol) -> sum {
            float dp = ol.ol_amount;
            char *x = static_data.all_item[ol.ol_i_id].i_data();
            if (memcmp(x, promo, plen) == 0) return sum(dp, dp);
            else return sum(dp, 0.0);
        };
        return ol_map::map_reduce(e.second, ol_f, Add());
    };

    sum revenues = delivery_map::map_reduce(delivery_range, date_f, Add(), 1);
    return revenues.second / (revenues.first+1) * 100;
}

double Q14time(maps m, bool verbose) {
    timer t;
    t.start();
    const char start[] = "2007-01-02";
    const char end[] = "2020-01-02";

    Q14_rtype result = Q14(m, start, end);
    double ret_tm = t.stop();

    cout << "Q14 : " << ret_tm << endl;

    if (verbose) {
        cout << "Q14:" << endl << result << endl;
    }
    return ret_tm;
}
