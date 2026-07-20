import unreal

def fix_assets():
    unreal.log("🚀 에셋 강제 세팅 스크립트 시작...")
    
    hud_path = "/Game/Moon/UI/WBP_CombatHUD.WBP_CombatHUD"
    dash_path = "/Game/Moon/GAS/Abilities/GA_Dash.GA_Dash"
    
    # 1. WBP_CombatHUD 부모 클래스 강제 변경
    hud_bp = unreal.EditorAssetLibrary.load_asset(hud_path)
    if hud_bp:
        hud_parent = unreal.load_class(None, "/Script/Moon.MoonCombatHUDWidget")
        # Blueprint의 부모 클래스 변경 (C++ 리플렉션 접근)
        try:
            unreal.BlueprintEditorLibrary.reparent_blueprint(hud_bp, hud_parent)
            unreal.EditorAssetLibrary.save_loaded_asset(hud_bp)
            unreal.log("✅ WBP_CombatHUD 부모 클래스 세팅 완료!")
        except Exception as e:
            unreal.log_error(f"부모 클래스 변경 실패: {e}")
    
    # 2. GA_Dash 강제 생성
    if not unreal.EditorAssetLibrary.does_asset_exist(dash_path):
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory_bp = unreal.BlueprintFactory()
        dash_parent = unreal.load_class(None, "/Script/Moon.MoonGameplayAbility_Dash")
        factory_bp.set_editor_property("parent_class", dash_parent)
        dash_bp = asset_tools.create_asset("GA_Dash", "/Game/Moon/GAS/Abilities", unreal.Blueprint, factory_bp)
        unreal.EditorAssetLibrary.save_loaded_asset(dash_bp)
        unreal.log("✅ GA_Dash 생성 완료!")
    else:
        unreal.log("✅ GA_Dash가 이미 정상적으로 존재합니다.")

    unreal.log("🎉 세팅이 1초만에 완료되었습니다! 이제 에셋을 열어서 확인해보세요.")

if __name__ == '__main__':
    fix_assets()
