//
//  Copyright (C) 2017 Microsoft.  All rights reserved.
//  See LICENSE file in the project root for full license information.
// 
#include "AppxFactory.hpp"
#include "UnicodeConversion.hpp"
#include "Exceptions.hpp"
#include "ZipObject.hpp"
#include "AppxPackageObject.hpp"

namespace MSIX {
    // IAppxFactory
    HRESULT STDMETHODCALLTYPE AppxFactory::CreatePackageWriter (
        IStream* outputStream,
        APPX_PACKAGE_SETTINGS* ,//settings, TODO: plumb this through
        IAppxPackageWriter** packageWriter)
    {
        return static_cast<HRESULT>(Error::NotImplemented);
    }

    HRESULT STDMETHODCALLTYPE AppxFactory::CreatePackageReader (
        IStream* inputStream,
        IAppxPackageReader** packageReader)
    {
        return ResultOf([&]() {
            ThrowErrorIf(Error::InvalidParameter, (packageReader == nullptr || *packageReader != nullptr), "Invalid parameter");
            ComPtr<IMSIXFactory> self;
            ThrowHrIfFailed(QueryInterface(UuidOfImpl<IMSIXFactory>::iid, reinterpret_cast<void**>(&self)));
            auto zip = ComPtr<IStorageObject>::Make<ZipObject>(self.Get(), inputStream);
            auto result = ComPtr<IAppxPackageReader>::Make<AppxPackageObject>(self.Get(), m_validationOptions, zip.Get());
            *packageReader = result.Detach();
        });
    }

    HRESULT STDMETHODCALLTYPE AppxFactory::CreateManifestReader(
        IStream* inputStream,
        IAppxManifestReader** manifestReader)
    {
        return ResultOf([&]() {
            // TODO: Implement
            throw Exception(Error::NotImplemented);
        });
    }

    HRESULT STDMETHODCALLTYPE AppxFactory::CreateBlockMapReader (
        IStream* inputStream,
        IAppxBlockMapReader** blockMapReader)
    {
        return ResultOf([&]() {
            ThrowErrorIf(Error::InvalidParameter, (
                inputStream == nullptr || 
                blockMapReader == nullptr || 
                *blockMapReader != nullptr
            ),"bad pointer.");

            ComPtr<IMSIXFactory> self;
            ThrowHrIfFailed(QueryInterface(UuidOfImpl<IMSIXFactory>::iid, reinterpret_cast<void**>(&self)));
            ComPtr<IStream> stream(inputStream);
            *blockMapReader = ComPtr<IAppxBlockMapReader>::Make<AppxBlockMapObject>(self.Get(), stream).Detach();
        });
    }

    HRESULT STDMETHODCALLTYPE AppxFactory::CreateValidatedBlockMapReader (
        IStream* inputStream,
        LPCWSTR signatureFileName,
        IAppxBlockMapReader** blockMapReader)
    {
        return ResultOf([&]() {
            ThrowErrorIf(Error::InvalidParameter, (
                inputStream == nullptr || 
                signatureFileName == nullptr ||
                *signatureFileName == '\0' ||
                blockMapReader == nullptr || 
                *blockMapReader != nullptr
            ),"bad pointer.");

            ComPtr<IMSIXFactory> self;
            ThrowHrIfFailed(QueryInterface(UuidOfImpl<IMSIXFactory>::iid, reinterpret_cast<void**>(&self)));
            auto stream = ComPtr<IStream>::Make<FileStream>(utf16_to_utf8(signatureFileName), FileStream::Mode::READ);
            auto signature = ComPtr<IVerifierObject>::Make<AppxSignatureObject>(self->GetValidationOptions(), stream.Get());
            auto validatedStream = signature->GetValidationStream("AppxBlockMap.xml", inputStream);
            *blockMapReader = ComPtr<IAppxBlockMapReader>::Make<AppxBlockMapObject>(self.Get(), validatedStream).Detach();
        });
    }

    HRESULT AppxFactory::MarshalOutString(std::string& internal, LPWSTR *result)
    {
        return ResultOf([&]() {
            ThrowErrorIf(Error::InvalidParameter, (result == nullptr || *result != nullptr), "bad pointer" );
            auto intermediate = utf8_to_utf16(internal);
            std::size_t countBytes = sizeof(wchar_t)*(internal.size()+1);
            *result = reinterpret_cast<LPWSTR>(m_memalloc(countBytes));
            ThrowErrorIfNot(Error::OutOfMemory, (*result), "Allocation failed!");
            std::memset(reinterpret_cast<void*>(*result), 0, countBytes);
            std::memcpy(reinterpret_cast<void*>(*result),
                        reinterpret_cast<void*>(const_cast<wchar_t*>(intermediate.c_str())),
                        countBytes - sizeof(wchar_t));
        });
    }

    HRESULT AppxFactory::MarshalOutBytes(std::vector<std::uint8_t>& data, UINT32* size, BYTE** buffer)
    {
        return ResultOf([&]{
            ThrowErrorIf(Error::InvalidParameter, (size==nullptr || buffer == nullptr || *buffer != nullptr), "Bad pointer");
            *size = static_cast<UINT32>(data.size());
            *buffer = reinterpret_cast<BYTE*>(m_memalloc(data.size()));
            ThrowErrorIfNot(Error::OutOfMemory, (*buffer), "Allocation failed");
            std::memcpy(reinterpret_cast<void*>(*buffer),
                        reinterpret_cast<void*>(data.data()),
                        data.size());
        });
    }

} // namespace MSIX 
