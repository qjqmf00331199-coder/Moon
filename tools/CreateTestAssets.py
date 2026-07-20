import unreal

def create_test_assets():
    unreal.log("🚀 에셋 자동 생성 스크립트 시작...")
    
    # 디렉토리 생성
    unreal.EditorAssetLibrary.make_directory("/Game/Moon/UI")
    unreal.EditorAssetLibrary.make_directory("/Game/Moon/GAS/Abilities")
    unreal.EditorAssetLibrary.make_directory("/Game/Moon/Maps")
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    
    # 1. WBP_CombatHUD 생성
    hud_path = "/Game/Moon/UI/WBP_CombatHUD.WBP_CombatHUD"
    if not unreal.EditorAssetLibrary.does_asset_exist(hud_path):
        factory_widget = unreal.WidgetBlueprintFactory()
        hud_parent = unreal.load_class(None, "/Script/Moon.MoonCombatHUDWidget")
        factory_widget.set_editor_property("parent_class", hud_parent)
        hud_bp = asset_tools.create_asset("WBP_CombatHUD", "/Game/Moon/UI", unreal.WidgetBlueprint, factory_widget)
        unreal.EditorAssetLibrary.save_asset(hud_bp.get_path_name())
        unreal.log("✅ WBP_CombatHUD 생성 완료!")
    else:
        unreal.log_warning("⚠️ WBP_CombatHUD가 이미 존재합니다.")

    # 2. GA_Dash 생성
    dash_path = "/Game/Moon/GAS/Abilities/GA_Dash.GA_Dash"
    if not unreal.EditorAssetLibrary.does_asset_exist(dash_path):
        factory_bp = unreal.BlueprintFactory()
        dash_parent = unreal.load_class(None, "/Script/Moon.MoonGameplayAbility_Dash")
        factory_bp.set_editor_property("parent_class", dash_parent)
        dash_bp = asset_tools.create_asset("GA_Dash", "/Game/Moon/GAS/Abilities", unreal.Blueprint, factory_bp)
        unreal.EditorAssetLibrary.save_asset(dash_bp.get_path_name())
        unreal.log("✅ GA_Dash 생성 완료!")
    else:
        unreal.log_warning("⚠️ GA_Dash가 이미 존재합니다.")

    unreal.log("🎉 기본 테스트 에셋 생성이 완료되었습니다! 에디터에서 열어보세요.")

if __name__ == '__main__':
    create_test_assets()
