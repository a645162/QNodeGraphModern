# Python scripts

`scripts/` 下只保留 Python 入口。从仓库根目录运行：

```powershell
python scripts/build.py --configuration Debug
python scripts/test.py --configuration Debug
python scripts/run.py basic
python scripts/package.py --configuration Release
python scripts/example/run_basic_demo.py
python scripts/example/run_image_pipeline_demo.py
```

`--build-dir` 可指定 Qt Creator 使用的构建目录，`--qt-prefix` 或
`QNODEGRAPH_QT_PREFIX` 可指定 Qt 安装路径。example 脚本默认先构建目标，
使用 `--no-build` 可直接运行已有二进制。
