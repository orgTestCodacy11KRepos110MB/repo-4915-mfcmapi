#pragma once
#include <core/model/mapiRowModel.h>

namespace propertybag
{
	enum class propBagFlags
	{
		None = 0x0000, // None
		Modified = 0x0001, // The property bag has been modified
		BackedByGetProps = 0x0002, // The property bag is rendering from a GetProps call
		AB = 0x0004, // The property bag represents an Address Book object
		Model = 0x0008, // The property bag supports models (can remove this when models fully implemented)
	};
	DEFINE_ENUM_FLAG_OPERATORS(propBagFlags)

	enum class propBagType
	{
		MAPIProp,
		Row,
		Account,
	};

	// TODO - Annotate for sal
	class IMAPIPropertyBag
	{
	public:
		virtual ~IMAPIPropertyBag() = default;

		virtual propBagFlags GetFlags() const = 0;
		virtual propBagType GetType() const = 0;
		virtual bool IsEqual(const std::shared_ptr<IMAPIPropertyBag> lpPropBag) const = 0;

		virtual _Check_return_ LPMAPIPROP GetMAPIProp() const = 0;

		// TODO: Should Commit take flags?
		virtual _Check_return_ HRESULT Commit() = 0;
		virtual _Check_return_ HRESULT GetAllProps(ULONG FAR* lpcValues, LPSPropValue FAR* lppPropArray) = 0;
		virtual _Check_return_ LPSPropValue GetOneProp(ULONG ulPropTag) = 0;
		virtual void FreeBuffer(LPSPropValue lpProp) = 0;
		virtual _Check_return_ HRESULT SetProps(ULONG cValues, LPSPropValue lpPropArray) = 0;
		virtual _Check_return_ HRESULT SetProp(LPSPropValue lpProp) = 0;
		virtual _Check_return_ HRESULT DeleteProp(ULONG ulPropTag) = 0;
		bool IsAB() { return (GetFlags() & propBagFlags::AB) == propBagFlags::AB; }

		// Model implementation
		// TODO: make this pure virtual after implemented by existing rows
		virtual _Check_return_ std::vector<std::shared_ptr<model::mapiRowModel>> GetAllModels() { return {}; }
		virtual _Check_return_ std::shared_ptr<model::mapiRowModel> GetOneModel(ULONG /*ulPropTag*/) { return {}; }
	};
} // namespace propertybag