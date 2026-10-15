# LimiterProcessor

[Исходник](../../External/Dsp/LimiterProcessor.cpp).

Native Output sample-peak limiter без lookahead.

Linked peak выбранных каналов задаёт ceiling/peak при превышении потолка.

Gain reduction немедленный, release80 мс; включённый limiter после gain применяет hard clamp.

Settled Target переводится в linear один раз на блок.

Снижение Target немедленно понижает потолок; повышение сглаживается10 мс.

После выключения остаточная GR восстанавливается.

Это отдельный Output limiter, не постоянные clamps Transient VST3.
