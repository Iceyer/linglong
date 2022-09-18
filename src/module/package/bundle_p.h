/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "bundle.h"

namespace linglong {
namespace package {

// __LITTLE_ENDIAN or __BIG_ENDIAN
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ELFDATANATIVE ELFDATA2LSB
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ELFDATANATIVE ELFDATA2MSB
#else
#error "Unknown machine endian"
#endif

// 16 bit system binary  swap
#define bswap16(value) ((((value)&0xff) << 8) | ((value) >> 8))
// 32 bit system binary  swap
#define bswap32(value) \
    (((uint32_t)bswap16((uint16_t)((value)&0xffff)) << 16) | (uint32_t)bswap16((uint16_t)((value) >> 16)))
// 64 bit system binary  swap
#define bswap64(value) \
    (((uint64_t)bswap32((uint32_t)((value)&0xffffffff)) << 32) | (uint64_t)bswap32((uint32_t)((value) >> 32)))

class BundlePrivate
{
public:
    // Bundle *q_ptr = nullptr;
    QString bundleFilePath;
    QString squashfsFilePath;
    QString bundleDataPath;
    int offsetValue;
    QString tmpWorkDir;
    QString buildArch;
    const QString linglongLoader = "/usr/libexec/linglong-loader";
    const QString configJson = "/info.json";

    explicit BundlePrivate(Bundle *parent)
    //    : q_ptr(parent)
    {
    }

    linglong::util::Error make(const QString &dataPath, const QString &outputFilePath);

    template<typename P>
    inline uint16_t file16ToCpu(uint16_t val, const P &ehdr)
    {
        if (ehdr.e_ident[EI_DATA] != ELFDATANATIVE)
            val = bswap16(val);
        return val;
    }

    template<typename P>
    uint32_t file32ToCpu(uint32_t val, const P &ehdr)
    {
        if (ehdr.e_ident[EI_DATA] != ELFDATANATIVE)
            val = bswap32(val);
        return val;
    }

    template<typename P>
    uint64_t file64ToCpu(uint64_t val, const P &ehdr)
    {
        if (ehdr.e_ident[EI_DATA] != ELFDATANATIVE)
            val = bswap64(val);
        return val;
    }

    // read elf64
    auto readElf64(FILE *fd, Elf64_Ehdr &ehdr) -> decltype(ehdr.e_shoff + (ehdr.e_shentsize * ehdr.e_shnum));

    // get elf offset size
    auto getElfSize(const QString elfFilePath) -> decltype(-1);

    linglong::util::Error push(const QString &bundleFilePath, const QString &repoUrl, const QString &repoChannel, bool force);
};
} // namespace package
} // namespace linglong
