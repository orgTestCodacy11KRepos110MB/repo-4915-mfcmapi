#include <UnitTest/stdafx.h>
#include <UnitTest/UnitTest.h>
#include <core/mapi/cache/namedProps.h>
#include <core/mapi/cache/namedPropCache.h>
#include <core/mapi/extraPropTags.h>
#include <core/utility/output.h>
#include <core/utility/registry.h>

namespace namedproptest
{
	TEST_CLASS(namedproptest)
	{
	public:
		// Without this, clang gets weird
		static const bool dummy_var = true;

		const std::vector<BYTE> sig1 = {1, 2, 3, 4};
		const std::vector<BYTE> sig2 = {5, 6, 7, 8, 9};

		const MAPINAMEID formStorageID = {const_cast<LPGUID>(&guid::PSETID_Common), MNID_ID, dispidFormStorage};
		const MAPINAMEID formStorageIDLog = {const_cast<LPGUID>(&guid::PSETID_Log), MNID_ID, dispidFormStorage};
		const MAPINAMEID formStorageName = {
			const_cast<LPGUID>(&guid::PSETID_Common), MNID_STRING, {.lpwstrName = const_cast<LPWSTR>(L"name")}};
		const MAPINAMEID formStorageName2 = {
			const_cast<LPGUID>(&guid::PSETID_Common), MNID_STRING, {.lpwstrName = const_cast<LPWSTR>(L"name2")}};
		const MAPINAMEID pageDirStreamID = {const_cast<LPGUID>(&guid::PSETID_Common), MNID_ID, dispidPageDirStream};
		const MAPINAMEID propDefStream = {const_cast<LPGUID>(&guid::PSETID_Common), MNID_ID, dispidPropDefStream};

		const std::shared_ptr<cache::namedPropCacheEntry> formStorage1 =
			cache::namedPropCacheEntry::make(&formStorageID, 0x1111, sig1);
		const std::shared_ptr<cache::namedPropCacheEntry> formStorage2 =
			cache::namedPropCacheEntry::make(&formStorageID, 0x1111, sig2);
		const std::shared_ptr<cache::namedPropCacheEntry> formStorageLog =
			cache::namedPropCacheEntry::make(&formStorageIDLog, 0x1111, sig1);

		const std::shared_ptr<cache::namedPropCacheEntry> formStorageProp =
			cache::namedPropCacheEntry::make(&formStorageName, 0x1111, sig1);
		const std::shared_ptr<cache::namedPropCacheEntry> formStorageProp1 =
			cache::namedPropCacheEntry::make(&formStorageName2, 0x1111, sig1);

		const std::shared_ptr<cache::namedPropCacheEntry> prop1 =
			cache::namedPropCacheEntry::make(&formStorageID, 0x1111);
		const std::shared_ptr<cache::namedPropCacheEntry> prop2 =
			cache::namedPropCacheEntry::make(&formStorageName, 0x2222);
		const std::shared_ptr<cache::namedPropCacheEntry> prop3 =
			cache::namedPropCacheEntry::make(&pageDirStreamID, 0x3333);
		const std::vector<std::shared_ptr<cache::namedPropCacheEntry>> ids1 = {prop1, prop2};
		const std::vector<std::shared_ptr<cache::namedPropCacheEntry>> ids2 = {prop3};

		TEST_CLASS_INITIALIZE(initialize)
		{
			unittest::init();
			if ((registry::debugTag & static_cast<DWORD>(output::dbgLevel::All)) ==
				static_cast<DWORD>(output::dbgLevel::All))
			{
				registry::debugTag |= static_cast<DWORD>(output::dbgLevel::NamedPropCache);
			}
		}

		TEST_METHOD(Test_Match)
		{
			// Test all forms of match
			Assert::AreEqual(true, formStorage1->match(formStorage1, true, true));
			Assert::AreEqual(true, formStorage1->match(formStorage1, false, true));
			Assert::AreEqual(true, formStorage1->match(formStorage1, true, false));
			Assert::AreEqual(true, formStorage1->match(formStorage1, false, false));

			// Odd comparisons
			Assert::AreEqual(
				true, formStorage1->match(cache::namedPropCacheEntry::make(&formStorageID, 0x1111, sig1), true, true));
			Assert::AreEqual(
				true, formStorage1->match(cache::namedPropCacheEntry::make(&formStorageID, 0x1112, sig1), false, true));
			Assert::AreEqual(
				true,
				formStorage1->match(cache::namedPropCacheEntry::make(&pageDirStreamID, 0x1111, sig1), true, false));
			Assert::AreEqual(
				true,
				formStorage1->match(cache::namedPropCacheEntry::make(&formStorageName, 0x1111, sig1), true, false));
			Assert::AreEqual(
				false,
				formStorage1->match(cache::namedPropCacheEntry::make(&formStorageName, 0x1111, sig1), true, true));

			// Should fail
			Assert::AreEqual(
				false, formStorage1->match(cache::namedPropCacheEntry::make(&formStorageID, 0x1110, sig1), true, true));
			Assert::AreEqual(
				false,
				formStorage1->match(cache::namedPropCacheEntry::make(&pageDirStreamID, 0x1111, sig1), true, true));
			Assert::AreEqual(false, formStorage1->match(nullptr, true, true));
			Assert::AreEqual(false, formStorage1->match(formStorage2, true, true));
			Assert::AreEqual(false, formStorage1->match(formStorageLog, true, true));

			// Compare given a signature, MAPINAMEID
			// _Check_return_ bool match(_In_ const std::vector<BYTE>& _sig, _In_ const MAPINAMEID& _mapiNameId) const;
			Assert::AreEqual(true, formStorage1->match(sig1, formStorageID));
			Assert::AreEqual(false, formStorage1->match(sig2, formStorageID));
			Assert::AreEqual(false, formStorage1->match(sig1, formStorageName));
			Assert::AreEqual(false, formStorage1->match(sig1, formStorageIDLog));
			Assert::AreEqual(false, formStorage1->match(sig1, pageDirStreamID));
			Assert::AreEqual(true, formStorageProp->match(sig1, formStorageName));
			Assert::AreEqual(false, formStorageProp->match(sig1, formStorageName2));

			// Compare given a signature and property ID (ulPropID)
			// _Check_return_ bool match(_In_ const std::vector<BYTE>& _sig, ULONG _ulPropID) const;
			Assert::AreEqual(true, formStorage1->match(sig1, 0x1111));
			Assert::AreEqual(false, formStorage1->match(sig1, 0x1112));
			Assert::AreEqual(false, formStorage1->match(sig2, 0x1111));

			// Compare given a id, MAPINAMEID
			// _Check_return_ bool match(ULONG _ulPropID, _In_ const MAPINAMEID& _mapiNameId) const noexcept;
			Assert::AreEqual(true, formStorage1->match(0x1111, formStorageID));
			Assert::AreEqual(false, formStorage1->match(0x1112, formStorageID));
			Assert::AreEqual(false, formStorage1->match(0x1111, pageDirStreamID));
			Assert::AreEqual(false, formStorage1->match(0x1111, formStorageName));
			Assert::AreEqual(false, formStorage1->match(0x1111, formStorageIDLog));
			Assert::AreEqual(true, formStorageProp->match(0x1111, formStorageName));
			Assert::AreEqual(false, formStorageProp->match(0x1111, formStorageName2));

			// String prop
			Assert::AreEqual(true, formStorageProp->match(formStorageProp, true, true));
			Assert::AreEqual(false, formStorageProp->match(formStorageProp1, true, true));
		}

		TEST_METHOD(Test_Cache)
		{
			cache::namedPropCache::add(ids1, sig1); // Add prop1, prop2 with signature
			cache::namedPropCache::add(ids1, {}); // Try to add them again without signature - this is a no-op
			cache::namedPropCache::add(ids2, {}); // Again, adding without a signature is a no op

			Assert::AreEqual(true, cache::namedPropCache::find(prop1, true, true)->match(prop1, true, true));
			Assert::AreEqual(true, cache::namedPropCache::find(prop2, true, true)->match(prop2, true, true));
			Assert::AreEqual(
				true,
				cache::namedPropCache::find(prop3, true, true)
					->match(cache::namedPropCacheEntry::empty(), true, true)); // Shouldn't find prop3 in the cache
			Assert::AreEqual(true, cache::namedPropCache::find(0x1111, formStorageID)->match(prop1, true, true));
			Assert::AreEqual(true, cache::namedPropCache::find(sig1, 0x1111)->match(prop1, true, true));
			Assert::AreEqual(true, cache::namedPropCache::find(sig1, formStorageID)->match(prop1, true, true));

			Assert::AreEqual(
				true,
				cache::namedPropCache::find(0x1110, formStorageID)
					->match(cache::namedPropCacheEntry::empty(), true, true));

			Assert::AreEqual(false, cache::namedPropCache::find(0x1110, formStorageID)->match(prop1, true, true));
			Assert::AreEqual(false, cache::namedPropCache::find(sig2, 0x1111)->match(prop1, true, true));
			Assert::AreEqual(false, cache::namedPropCache::find(sig2, formStorageID)->match(prop1, true, true));

			// None of these should match prop3 since we never cached it!
			Assert::AreEqual(
				true,
				cache::namedPropCache::find(0x3333, pageDirStreamID)
					->match(cache::namedPropCacheEntry::empty(), true, true));
			Assert::AreEqual(
				true, cache::namedPropCache::find({}, 0x3333)->match(cache::namedPropCacheEntry::empty(), true, true));
			Assert::AreEqual(
				true,
				cache::namedPropCache::find(std::vector<BYTE>{}, pageDirStreamID)
					->match(cache::namedPropCacheEntry::empty(), true, true));

			cache::namedPropCache::add(ids2, sig2); // Now add prop3 with a signature and our lookups should work
			Assert::AreEqual(
				true,
				cache::namedPropCache::find(prop3, true, true)
					->match(prop3, true, true)); // Shouldn't find prop3 in the cache
			Assert::AreEqual(true, cache::namedPropCache::find(0x3333, pageDirStreamID)->match(prop3, true, true));
			Assert::AreEqual(true, cache::namedPropCache::find({}, 0x3333)->match(prop3, true, true));
			Assert::AreEqual(
				true, cache::namedPropCache::find(std::vector<BYTE>{}, pageDirStreamID)->match(prop3, true, true));
		}

		TEST_METHOD(Test_Valid)
		{
			Assert::AreEqual(false, cache::namedPropCacheEntry::valid(cache::namedPropCacheEntry::empty()));
			Assert::AreEqual(
				true,
				cache::namedPropCacheEntry::valid(cache::namedPropCacheEntry::make(&formStorageName, 0x1111, sig1)));
			Assert::AreEqual(
				true, cache::namedPropCacheEntry::valid(cache::namedPropCacheEntry::make(&formStorageName, 0x1111)));
			Assert::AreEqual(
				false, cache::namedPropCacheEntry::valid(cache::namedPropCacheEntry::make(&formStorageName, 0)));
			Assert::AreEqual(
				false, cache::namedPropCacheEntry::valid(cache::namedPropCacheEntry::make(nullptr, 0x1111)));
			Assert::AreEqual(false, cache::namedPropCacheEntry::valid(cache::namedPropCacheEntry::make(nullptr, 0)));
			Assert::AreEqual(false, cache::namedPropCacheEntry::valid(nullptr));
			Assert::AreEqual(
				false,
				cache::namedPropCacheEntry::valid(cache::namedPropCacheEntry::make(&formStorageName, PT_ERROR, sig1)));
		}

		TEST_METHOD(Test_Names)
		{
			cache::namedPropCache::add(ids1, sig1);
			cache::namedPropCache::add(ids2, {});

			Assert::AreEqual(false, cache::namedPropCacheEntry::valid(cache::namedPropCacheEntry::empty()));
			const auto sig1bin = SBinary{static_cast<ULONG>(sig1.size()), const_cast<BYTE*>(sig1.data())};
			const auto name1 = cache::NameIDToStrings(0x1111, nullptr, &formStorageID, &sig1bin, false);
			Assert::AreEqual(name1.name, std::wstring{L"id: 0x850F=34063 = dispidFormStorage"});
			Assert::AreEqual(name1.guid, std::wstring{L"{00062008-0000-0000-C000-000000000046} = PSETID_Common"});
			Assert::AreEqual(name1.dasl, std::wstring{L"id/{00062008-0000-0000-C000-000000000046}/850F1111"});
			Assert::AreEqual(name1.bestPidLid, std::wstring{L"dispidFormStorage"});

			const auto name2 = cache::NameIDToStrings(0x1111, nullptr, {}, &sig1bin, false);
			Assert::AreEqual(name2.name, std::wstring{L""});
			Assert::AreEqual(name2.guid, std::wstring{L""});
			Assert::AreEqual(name2.dasl, std::wstring{L""});
			Assert::AreEqual(name2.bestPidLid, std::wstring{L""});

			const auto propNames = cache::NameIDToPropNames(&propDefStream);
			Assert::AreEqual(propNames[0], std::wstring{L"PidLidPropertyDefinitionStream"});
			Assert::AreEqual(propNames[1], std::wstring{L"dispidPropDefStream"});

			const auto propNames2 = cache::NameIDToPropNames({});
			Assert::AreEqual(propNames2.empty(), true);
		}

		TEST_METHOD(Test_String)
		{
			Assert::AreEqual(
				std::wstring(L"ulKind=MNID_ID, lID=850F, guid={00062008-0000-0000-C000-000000000046} = PSETID_Common"),
				strings::MAPINAMEIDToString(formStorageID));
			Assert::AreEqual(
				std::wstring(L"ulKind=MNID_STRING, lpwstrName='name', guid={00062008-0000-0000-C000-000000000046} = "
							 L"PSETID_Common"),
				strings::MAPINAMEIDToString(formStorageName));
			const auto badID = MAPINAMEID{const_cast<LPGUID>(&guid::PSETID_Common), 3, dispidFormStorage};
			Assert::AreEqual(std::wstring(L""), strings::MAPINAMEIDToString(badID));
		}
	};
} // namespace namedproptest
