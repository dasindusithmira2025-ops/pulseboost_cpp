from functools import lru_cache
from pathlib import Path

from pydantic import Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    anthropic_api_key: str = Field(default="", alias="ANTHROPIC_API_KEY")
    poll_interval_seconds: int = Field(default=3, alias="POLL_INTERVAL_SECONDS")
    db_path: Path = Field(default=Path("data/memory.db"), alias="DB_PATH")
    vector_path: Path = Field(default=Path("data/vectors"), alias="VECTOR_PATH")
    log_level: str = Field(default="INFO", alias="LOG_LEVEL")
    auto_heal_mode: str = Field(default="suggest", alias="AUTO_HEAL_MODE")
    baseline_min_samples: int = Field(default=50, alias="BASELINE_MIN_SAMPLES")
    max_history_days: int = Field(default=30, alias="MAX_HISTORY_DAYS")
    cors_origins: str = Field(default="", alias="CORS_ORIGINS")
    executor_dry_run: bool = Field(default=True, alias="EXECUTOR_DRY_RUN")
    chat_daily_limit_free: int = Field(default=10, alias="CHAT_DAILY_LIMIT_FREE")
    default_plan: str = Field(default="free", alias="DEFAULT_PLAN")
    enable_auto_heal: bool = Field(default=False, alias="ENABLE_AUTO_HEAL")
    machine_id: str = Field(default="local-machine", alias="MACHINE_ID")
    machine_name: str = Field(default="Primary Machine", alias="MACHINE_NAME")
    auth_enabled: bool = Field(default=False, alias="AUTH_ENABLED")
    auth_dev_mode: bool = Field(default=False, alias="AUTH_DEV_MODE")  # default OFF
    app_version: str = Field(default="2.4.0", alias="APP_VERSION")
    stripe_public_key: str = Field(default="", alias="STRIPE_PUBLIC_KEY")
    stripe_price_pro: str = Field(default="", alias="STRIPE_PRICE_PRO")
    stripe_price_team: str = Field(default="", alias="STRIPE_PRICE_TEAM")
    website_authority_url: str = Field(default="", alias="WEBSITE_AUTHORITY_URL")
    website_signin_url: str = Field(default="", alias="WEBSITE_SIGNIN_URL")
    website_create_account_url: str = Field(default="", alias="WEBSITE_CREATE_ACCOUNT_URL")
    website_manage_subscription_url: str = Field(default="", alias="WEBSITE_MANAGE_SUBSCRIPTION_URL")
    history_rollup_interval_cycles: int = Field(default=20, alias="HISTORY_ROLLUP_INTERVAL_CYCLES")
    history_window_hours_3s: int = Field(default=24, alias="HISTORY_WINDOW_HOURS_3S")
    history_window_days_1m: int = Field(default=7, alias="HISTORY_WINDOW_DAYS_1M")
    history_window_days_5m: int = Field(default=30, alias="HISTORY_WINDOW_DAYS_5M")
    desktop_runtime: str = Field(default="electron", alias="DESKTOP_RUNTIME")

    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        case_sensitive=False,
        extra="ignore",
    )

    @property
    def resolved_db_path(self) -> Path:
        return Path(self.db_path)

    @property
    def resolved_vector_path(self) -> Path:
        return Path(self.vector_path)

    @property
    def cors_origin_list(self) -> list[str]:
        return [origin.strip() for origin in self.cors_origins.split(",") if origin.strip()]


@lru_cache(maxsize=1)
def get_settings() -> Settings:
    return Settings()
