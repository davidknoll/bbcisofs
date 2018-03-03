        .title ISO-9660 (CDFS) for the BBC Micro
        .module defs
        .r6500

; MOS calls
		.area MOS (ABS)
		.org 0xFFB9
OSRDRM::
		.org 0xFFBF
OSEVEN::
		.org 0xFFCE
OSFIND::
		.org 0xFFD1
OSGBPB::
		.org 0xFFD4
OSBPUT::
		.org 0xFFD7
OSBGET::
		.org 0xFFDA
OSARGS::
		.org 0xFFDD
OSFILE::
		.org 0xFFE0
OSRDCH::
		.org 0xFFE3
OSASCI::
		.org 0xFFE7
OSNEWL::
		.org 0xFFEE
OSWRCH::
		.org 0xFFF1
OSWORD::
		.org 0xFFF4
OSBYTE::
		.org 0xFFF7
OSCLI::

; IDE interface
		.area FRED (ABS)
		.org 0xFC40
IDEDATL::
		.ds 1
IDEERR::
IDEFEAT::
		.ds 1
IDESCCT::
		.ds 1
IDESECT::
		.ds 1
IDECYLL::
		.ds 1
IDECYLH::
		.ds 1
IDEHEAD::
		.ds 1
IDESTAT::
IDECMD::
		.ds 1
IDEDATH::
		.ds 6
IDEALST::
IDEDCTL::
		.ds 1
IDEDADR::

		.end
