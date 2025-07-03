#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Types.h"
#include "Components/TimelineComponent.h"
#include "TPSCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, NewStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaStateChanged, EStaminaState, NewState);

UCLASS(Blueprintable)
class TPS_API ATPSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATPSCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }

private:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

	
	UPROPERTY()
	float CurrentStamina;

	UPROPERTY()
	float StaminaRecoveryTimer;

	UPROPERTY()
	bool bIsSprintingInternal;

	UPROPERTY()
	EStaminaState CurrentStaminaState;

	
	UPROPERTY()
	class UTimelineComponent* SprintTimeline;

	UPROPERTY()
	class UCurveFloat* SprintCurve;

	float BaseSpeed;
	float TargetSpeed;

	
	void UpdateStaminaState();
	float GetSpeedModifier() const;
	void OnSprintTimelineUpdate(float Value);
	void OnSprintTimelineFinished();
	bool IsMovingForward() const;

	
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SprintPressed();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SprintReleased();

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	EMovementState MovementState = EMovementState::Run_State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FCharacterSpeed MovementSpeedInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FStaminaSettings StaminaSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ForwardSprintThreshold = 0.7f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool SprintRunEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool WalkEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool AimEnabled = false;

	
	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FOnStaminaChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FOnStaminaStateChanged OnStaminaStateChanged;

	
	UFUNCTION()
	void InputAxisY(float Value);

	UFUNCTION()
	void InputAxisX(float Value);

	UPROPERTY()
	float AxisX = 0.0f;

	UPROPERTY()
	float AxisY = 0.0f;

	
	UFUNCTION()
	void MovementTick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void CharacterUpdate();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ChangeMovementState();

	// Stamina
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void UpdateStamina(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool CanSprint() const;

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void StopSprint();

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const { return StaminaSettings.MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetStaminaPercentage() const;

	UFUNCTION(BlueprintPure, Category = "Stamina")
	EStaminaState GetStaminaState() const { return CurrentStaminaState; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool IsSprinting() const { return bIsSprintingInternal; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsMovingForwardDirection() const { return IsMovingForward(); }

protected:
	virtual void BeginPlay() override;
};