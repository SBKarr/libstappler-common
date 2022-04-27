/**
 Copyright (c) 2021-2022 Roman Katuntsev <sbkarr@stappler.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#ifndef MODULES_CRYPTO_SPCRYPTO_H_
#define MODULES_CRYPTO_SPCRYPTO_H_

#include "SPBytesView.h"
#include "SPIO.h"
#include "SPSha.h"

struct gnutls_pubkey_st;
typedef struct gnutls_pubkey_st *gnutls_pubkey_t;

struct gnutls_privkey_st;
typedef struct gnutls_privkey_st *gnutls_privkey_t;

namespace stappler::crypto {

class PublicKey;

enum class SignAlgorithm {
	RSA_SHA256,
	RSA_SHA512,
	ECDSA_SHA256,
	ECDSA_SHA512,
};

enum class KeyBits {
	_1024,
	_2048,
	_4096
};

using AesKey = std::array<uint8_t, 32>;

class PrivateKey {
public:
	PrivateKey();
	PrivateKey(BytesView, const CoderSource & passwd = CoderSource());
	~PrivateKey();

	bool generate(KeyBits = KeyBits::_2048);
	bool import(BytesView, const CoderSource & passwd = CoderSource());

	PublicKey exportPublic() const;

	gnutls_privkey_t getKey() const { return _key; }

	operator bool () const { return _valid; }

	template <typename Interface>
	auto exportPem() -> typename Interface::BytesType;

	template <typename Interface>
	auto exportDer() -> typename Interface::BytesType;

	template <typename Interface>
	auto sign(BytesView, SignAlgorithm = SignAlgorithm::RSA_SHA512) -> typename Interface::BytesType;

protected:
	bool _loaded = false;
	bool _valid = false;
	gnutls_privkey_t _key;
};

class PublicKey {
public:
	PublicKey();
	PublicKey(BytesView);
	PublicKey(const PrivateKey &);
	~PublicKey();

	bool import(BytesView);
	bool import(BytesView, BytesView);

	gnutls_pubkey_t getKey() const { return _key; }

	operator bool () const { return _valid; }

	template <typename Interface>
	auto exportPem() -> typename Interface::BytesType;

	template <typename Interface>
	auto exportDer() -> typename Interface::BytesType;

	bool verify(BytesView data, BytesView signature, SignAlgorithm);

protected:
	bool _loaded = false;
	bool _valid = false;
	gnutls_pubkey_t _key;
};

// convert public openssh key into DER format
template <typename Interface>
auto convertOpenSSHKey(const StringView &) -> typename Interface::BytesType;

template <typename Interface>
auto encryptAes(const AesKey &, BytesView, uint32_t version) -> typename Interface::BytesType;

template <typename Interface>
auto decryptAes(const AesKey &, BytesView) -> typename Interface::BytesType;

AesKey makeAesKey(BytesView pkey, BytesView hash, uint32_t version);
uint32_t getAesVersion(BytesView);

template <typename Interface>
auto sign(gnutls_privkey_t, BytesView, SignAlgorithm) -> typename Interface::BytesType;

template <typename Interface>
auto exportPem(gnutls_pubkey_t) -> typename Interface::BytesType;

template <typename Interface>
auto exportDer(gnutls_pubkey_t) -> typename Interface::BytesType;

template <typename Interface>
auto exportPem(gnutls_privkey_t) -> typename Interface::BytesType;

template <typename Interface>
auto exportDer(gnutls_privkey_t) -> typename Interface::BytesType;

template <typename Interface>
auto PrivateKey::exportPem() -> typename Interface::BytesType {
	return crypto::exportPem<Interface>(_key);
}

template <typename Interface>
auto PrivateKey::exportDer() -> typename Interface::BytesType {
	return crypto::exportDer<Interface>(_key);
}

template <typename Interface>
auto PrivateKey::sign(BytesView data, SignAlgorithm algo) -> typename Interface::BytesType {
	return crypto::sign<Interface>(_key, data, algo);
}

template <typename Interface>
auto PublicKey::exportPem() -> typename Interface::BytesType {
	return crypto::exportPem<Interface>(_key);
}

template <typename Interface>
auto PublicKey::exportDer() -> typename Interface::BytesType {
	return crypto::exportDer<Interface>(_key);
}

}

#endif /* MODULES_CRYPTO_SPCRYPTO_H_ */
