#pragma once

#include "Templates/SharedPointer.h"

#include "OdinCaptureMedia.h"
#include "OdinJsonObject.h"

#include <memory>

#include "OdinPlaybackMedia.h"

#include "OdinRoom.generated.h"

UCLASS(ClassGroup = Odin, meta = (BlueprintSpawnableComponent))
class UOdinRoom : public UObject
{
    GENERATED_UCLASS_BODY()

  public:
    UFUNCTION(BlueprintCallable,
              meta     = (DisplayName = "Construct a Room", HidePin = "WorldContextObject",
                      DefaultToSelf = "WorldContextObject"),
              Category = "Odin|Room")
    static UOdinRoom *ConstructRoom(UObject *WorldContextObject, const FString &url,
                                    const FString &token);

    UFUNCTION(BlueprintCallable, Category = "Odin|Function|Room")
    void OdinRoom_AddMedia(UOdinCaptureMedia *media);

    UFUNCTION(BlueprintCallable, Category = "Odin|Function|Room")
    void OdinRoom_Join(const TArray<uint8> &user_data);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinRoomJoined, int64, peerId, UOdinJsonObject *, roomUserData);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Callbacks|Room")
    FOdinRoomJoined onRoomJoined;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinMediaAdded, int64, peerId,
                                                   UOdinPlaybackMedia *, media, UOdinJsonObject *,
                                                   properties);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Callbacks|Room")
    FOdinMediaAdded onMediaAdded;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinPeerJoined, int64, peerId, UOdinJsonObject *,
                                                 userData);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Callbacks|Room")
    FOdinPeerJoined onPeerJoined;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdinPeerUserDataChanged, int64, peerId,
                                                 UOdinJsonObject *, userData);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Callbacks|Room")
    FOdinPeerJoined onPeerUserDataChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinPeerLeft, int64, peerId);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Callbacks|Room")
    FOdinPeerLeft onPeerLeft;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdinMediaRemoved, int32, mediaId);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Callbacks|Room")
    FOdinMediaRemoved onMediaRemoved;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdinMediaActiveStateChanged, int64, peerId, int32, mediaId,
                                                  bool, active);
    UPROPERTY(BlueprintAssignable, Category = "Odin|Callbacks|Room")
    FOdinMediaActiveStateChanged onMediaActiveStateChanged;

  public:
    void SetInitParams(const FString &url, const FString &room_id);

  protected:
    void BeginDestroy() override;

  private:
    FString Url;
    FString RoomId;

    std::shared_ptr<struct OdinRoom> room_;

    TArray<UOdinCaptureMedia *> capture_medias_;

    void HandleOdinEvent(const struct OdinEvent *event);
};
