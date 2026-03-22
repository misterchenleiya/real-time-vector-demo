# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- 初始化 `real-time-vector-demo` MVP 工程骨架，包含核心库、Qt Widgets GUI、CLI 和基础测试入口。
- 新增基于 JSON 的发送端 / 接收端配置样例，用于本地多实例 UDP fan-out 联调。
- 新增 MVP 架构 ADR，明确媒体处理、路径稳定化、场景路由和 UDP 输出方案。
- 新增 `Makefile`，提供统一的 `build`、`run` 和 `clean` 入口，便于本地构建和启动 DEMO。

### Changed

- 将 MVP 输出模型收敛为“发送端生成设备矢量帧并通过本地回环 UDP 分发给多个 DEMO 实例显示”。
- 将 `README` 翻译为英文，并补充基于 `make` 的构建和运行说明。
- 在 `README` 顶部新增压缩后的 DEMO 运行截图，便于用户直接在 GitHub 仓库首页查看当前界面效果。
- 将本地 sender 配置的 `maxPacketSize` 提高到 `8192`，降低 loopback 演示时较大路径帧触发多分片的概率。
- 将默认 `make run` 调整为一次启动两个 receiver GUI 窗口和一个 sender GUI 窗口，便于本机三窗口联调。
- 将 DEMO GUI 的按钮、状态、日志和空态提示翻译为英文，并为 sender / receiver 窗口设置可区分的标题。
- 将 GUI 窗口调整为固定尺寸；加载图片或视频后立即预览；sender 点击 `Stop` 时向 receiver 下发清空帧以清除显示内容。
