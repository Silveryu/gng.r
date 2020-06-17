//
// Created by silveryu on 10/11/19.
//

#include <libnet.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/IndexHNSW.h>
#include <faiss/index_factory.h>
#include <faiss/Index.h>
#include <faiss/MetaIndexes.h>


int main(){
    int d = 64;                            // dimension
    int nb = 1000;                       // database size
    int nq = 10;                        // nb of queries

    float *xb = new float[d * nb];
    float *xq = new float[d * nq];

    for(int i = 0; i < nb; i++) {
        for(int j = 0; j < d; j++)
            xb[d * i + j] = drand48();
        xb[d * i] += i / 1000.;
    }

    for(int i = 0; i < nq; i++) {
        for(int j = 0; j < d; j++)
            xq[d * i + j] = drand48();
        xq[d * i] += i / 1000.;
    }

    //faiss::IndexIDMap * ann;
    // do these tests first
    faiss::IndexIDMap* ann =  (faiss::IndexIDMap*) faiss::index_factory(d, "IDMap, LSH", faiss::METRIC_L2);


    std::cout << typeid(ann).name()  << std::endl;
    //ann->search
    //ann->hnsw.efConstruction = 36;
    //ann->hnsw.efSearch= 36;



    printf("is_trained = %s\n", ann->is_trained ? "true" : "false");
    //index_flat.add(nb, xb);  // add vectors to the index

    long ids[nb];

    for (int i = 0; i < nb; i++){
        ids[i] = i;
    }

    ann->add_with_ids(nb, xb, ids);
    printf("ntotal = %ld\n", ann->ntotal);



    long idsRm[] = {3};
    //ann->remove_ids(faiss::IDSelectorBatch(1, idsRm));


    ann->remove_ids(faiss::IDSelectorBatch(1, idsRm));

    int k = 4;

    {       // search xq
        long *I = new long[k * nq];
        float *D = new float[k * nq];

        ann->search(nq, xq, k, D, I);

        // print results
        printf("I (5 first results)=\n");
        for(int i = 0; i < 5; i++) {
            for(int j = 0; j < k; j++)
                printf("%5ld ", I[i * k + j]);
            printf("\n");
        }

        printf("I (5 last results)=\n");
        for(int i = nq - 5; i < nq; i++) {
            for(int j = 0; j < k; j++)
                printf("%5ld ", I[i * k + j]);
            printf("\n");
        }

        delete [] I;
        delete [] D;
    }

}