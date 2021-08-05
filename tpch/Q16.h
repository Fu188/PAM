// brand, type, size, supplier_count
using Q16_elt = tuple<char *, char *, float, int>;
using Q16_rtype = sequence<Q16_elt>;

Q16_rtype Q16(maps m, const char *type) {
    const int type_len = strlen(type);
    const char complain[] = "bad";

    // part number and part count
    using rtype = pair<uint, int>;
    item_to_supp_map &psim = m.ism2;

    // hash table for parts kept by brand, type and size,
    // and with integer count
    struct hash_item {
        using eType = rtype;
        using kType = uint;
        Item *prts;

        hash_item(Item *prts) : prts(prts) {}

        eType empty() { return rtype((uint) - 1, 0.0); }

        kType getKey(eType v) { return v.first; }

        size_t hash_string(char *s) {
            size_t hash = 0;
            while (*s) hash = *s++ + (hash << 6) + (hash << 16) - hash;
            return hash;
        }

        size_t hash(kType v) {
            Item &p = prts[v];
            return pbbs::hash64(hash_string(p.i_data()));
        }

        // compare on brand, then type, then size
        int cmp(kType a, kType b) {
            Item &pa = prts[a];
            Item &pb = prts[b];
            int type_cmp = strcmp(pa.i_data(), pb.i_data());
            if (type_cmp != 0) return type_cmp;
            return 0;
        }

        bool replaceQ(eType v, eType b) { return 1; }

        eType update(eType v, eType b) { return rtype(v.first, v.second + b.second); }

        bool cas(eType *p, eType &o, eType &n) {
            return pbbs::atomic_compare_and_swap(p, o, n);
        }
    };

    auto htable = hash_item(static_data.all_item);
    pbbs::Table <hash_item> T(psim.size() / 6, htable, 2);

    auto map_item = [&](item_to_supp_map::E &e, size_t i) {
        Item &p = e.second.first;
        int item_key = p.i_id;

        // exclude if type matches
        if (strncmp(p.i_data(), type, type_len) == 0) return;

        // count suppliers with no customer complaints
        auto map_supp = [&](item_supp_and_orderline_map::E &e) -> int {
            //Supplier& s = e.second.first;
            uint suppkey = e.second.first.s_su_suppkey;
            Supplier &s = static_data.all_supp[suppkey];
            char *p = strstr(s.su_comment(), complain);
            if (p == NULL) return 1;
            else return 0;
        };
        int count = item_supp_and_orderline_map::map_reduce(e.second.second, map_supp, Add<int>());

        // update hash table by adding count
        T.update(rtype(item_key, count));
    };

    // go over all parts adding to hash table
    item_to_supp_map::map_index(psim, map_item);

    // extract entries from hash table
    sequence <rtype> res = T.entries();

    // sort by, count (desc), brand, type, size
    Item *items = static_data.all_item;
    auto less = [&](rtype a, rtype b) {
        if (a.second > b.second) return true;
        else if (a.second == b.second)
            return (htable.cmp(a.first, b.first) == -1);
        else return false;
    };
    sequence <rtype> sorted_res = pbbs::sample_sort(res, less);

    // generate result sequence
    auto to_res = [&](size_t i) {
        Item p = items[sorted_res[i].first];
        int count = sorted_res[0].second;
        char str_cp[4];
        copy_string(str_cp, p.i_data()+1, 3);
        return Q16_elt(p.i_name(), str_cp, p.i_price, count);
    };
    return sequence<Q16_elt>(sorted_res.size(), to_res);
}

double Q16time(maps m, bool verbose) {
    const char type[] = "zz";
    timer t;
    t.start();

    Q16_rtype result = Q16(m, type);

    double ret_tm = t.stop();
    cout << "Q16 : " << ret_tm << endl;

    if (verbose) {
        Q16_elt r = result[0];
        cout << "Q16:" << endl
             << get<0>(r) << ", " << get<1>(r) << ", "
             << get<2>(r) << ", " << get<3>(r) << endl;
    }
    return ret_tm;
}
