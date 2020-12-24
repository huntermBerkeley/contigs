#pragma once

#include "kmer_t.hpp"
#include "hash_map.hpp"
#include <upcxx/upcxx.hpp>

struct UpcHashMap {
    //each processor involved will allocate
    upcxx::dist_object<HashMap> map;


    size_t size() const noexcept;

    UpcHashMap(size_t size);

    // Most important functions: insert and retrieve
    // k-mers from the hash table.
    void insert(const kmer_pair& kmer);
    upcxx::future<kmer_pair> find(const pkmer_t& key_kmer);

    // Helper functions

    // Write and read to a logical data slot in the table.
    void write_slot(uint64_t slot, const kmer_pair& kmer);
    kmer_pair read_slot(uint64_t slot);

    // Request a slot or check if it's already used.
    bool request_slot(uint64_t slot);
    bool slot_used(uint64_t slot);
};

UpcHashMap::UpcHashMap(size_t size) {


    //round up to guarantee hashhmap >= size
    size_t my_size = (size + upcxx::rank_n() - 1) / upcxx::rank_n();

    //will this work?
    map = upcxx::dist_object<HashMap>(my_size);

    //remotes should build hashmap
    HashMap my_hashmap(my_size);
    //and mark
    *map = my_hashmap;
}

void UpcHashMap::insert(const kmer_pair& kmer) {

    uint64_t hash = kmer.hash() % upcxx::rank_n();


   upcxx::rpc(hash,

      [](
        upcxx::dist_object<HashMap> & map,
        const kmer_pair &kmer
      ) -> void {

        //return insert
        bool success = map->insert(kmer);

        if (!success){
          throw std::runtime_error("Error: HashMap is full!");
        }
        return;

      }, map, kmer);

      return;
    }


upcxx::future<kmer_pair> UpcHashMap::find(const pkmer_t& key_kmer) {


    uint64_t hash = key_kmer.hash() % upcxx::rank_n();

    kmer_pair toReturn;

    return upcxx::rpc(hash,

    [](
      upcxx::dist_object<HashMap>  &map,
      const kmer_pair  & key_kmer
    ) -> kmer_pair {

      //init kmer to Return
      kmer_pair toReturn;

      bool success = map->find(key_kmer, &toReturn);

      if (!success){
          throw std::runtime_error("Error: Kmer not found!");
      }

      return toReturn;

    },

    map, key_kmer);

}
