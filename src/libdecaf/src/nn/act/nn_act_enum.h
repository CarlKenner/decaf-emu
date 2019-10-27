#ifndef NN_ACT_ENUM_H
#define NN_ACT_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(nn)
ENUM_NAMESPACE_ENTER(act)

ENUM_BEG(InfoType, int32_t)
   ENUM_VALUE(NumOfAccounts,              1)
   ENUM_VALUE(SlotNo,                     2)
   ENUM_VALUE(DefaultAccount,             3)
   ENUM_VALUE(NetworkTimeDifference,      4)
   ENUM_VALUE(PersistentId,               5)
   ENUM_VALUE(LocalFriendCode,            6)
   ENUM_VALUE(Mii,                        7)
   ENUM_VALUE(AccountId,                  8)
   ENUM_VALUE(Birthday,                   10)
   ENUM_VALUE(Country,                    11)
   ENUM_VALUE(PrincipalId,                12)
   ENUM_VALUE(Gender,                     19)
   ENUM_VALUE(ParentalControlSlot,        22)
   ENUM_VALUE(SimpleAddressId,            23)
   ENUM_VALUE(IsCommitted,                26)
   ENUM_VALUE(NfsPassword,                28)
   ENUM_VALUE(ApplicationUpdateRequired,  35)
   ENUM_VALUE(DeviceHash,                 43)
   ENUM_VALUE(NetworkTime,                46)
   ENUM_VALUE(MiiName,                    27)
ENUM_END(InfoType)

ENUM_NAMESPACE_EXIT(act)
ENUM_NAMESPACE_EXIT(nn)

#include <common/enum_end.inl>

#endif // ifdef NN_ACT_ENUM_H
