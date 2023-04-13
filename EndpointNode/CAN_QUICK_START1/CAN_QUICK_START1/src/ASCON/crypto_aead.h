int crypto_aead_encrypt(unsigned char *c, unsigned long *clen,
                        const unsigned char *m, unsigned long mlen,
                        const unsigned char *ad, unsigned long adlen,
                        const unsigned char *nsec, const unsigned char *npub,
                        const unsigned char *k);

int crypto_aead_decrypt(unsigned char *m, unsigned long *mlen,
                        unsigned char *nsec, const unsigned char *c,
                        unsigned long clen, const unsigned char *ad,
                        unsigned long adlen, const unsigned char *npub,
                        const unsigned char *k);
