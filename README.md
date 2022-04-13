# 4Players ODIN Unreal SDK

ODIN is a cross-platform software development kit (SDK) that enables developers to integrate real-time chat technology into multiplayer games, apps and websites.

The ODIN SDK for Unreal enables you to add real-time VoIP communication to your Unreal Engine 4/5 game, thus making it more social and interactive.

[Online Documentation](https://developers.4players.io/odin)

## Prerequisites

- Unreal Engine 4.10 or any later version (including 5.x)

Internally, the plugin is built and tested with Unreal Engine 4.27.

## Getting Started

To check out the SDK and use it as a project plugin, clone the git repo into a working directory of your choice.

This repository uses [LFS](https://git-lfs.github.com) (large file storage) to manage pre-compiled binaries. Note that a standard clone of the repository might only retrieve the metadata about these files managed with LFS. In order to retrieve the actual data with LFS, please follow these steps:

1. Clone the repository:  
   `git clone https://github.com/4Players/odin-sdk-unreal.git`

2. Cache the actual LFS data on your local machine:  
   `git lfs fetch`

3. Replaces the metadata in the binary files with their actual contents:  
   `git lfs checkout`

Next, unzip the files into a new **Odin** sub-folder under your projects **Plugins** folder. It should end up looking like `/MyProject/Plugins/Odin/`.

If you're new to plugins in UE4, you can find lots of information [right here](https://unrealcommunity.wiki/an-introduction-to-ue4-plugins-v1v672wq).

## Troubleshooting

Contact us through the listed methods below to receive answers to your questions and learn more about ODIN.

### Discord

Join our official Discord server to chat with us directly and become a part of the 4Players ODIN community.

[![Join us on Discord](https://developers.4players.io/images/join_discord.png)](https://discord.gg/9yzdJNUGZS)

### Twitter

Have a quick question? Tweet us at [@4PlayersBiz](https://twitter.com/4PlayersBiz) and we’ll help you resolve any issues.

### Email

Don’t use Discord or Twitter? Send us an [email](mailto:odin@4players.io) and we’ll get back to you as soon as possible.
