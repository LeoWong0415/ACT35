#ifdef SW_I2C_SLAVE
	else if( !stricmp( argv[0], "i2c" ) ) {

		extern BYTE dbg_sw_i2c_index[];
		extern BYTE dbg_sw_i2c_devid[];
		extern BYTE dbg_sw_i2c_regidx[];
		extern BYTE dbg_sw_i2c_data[];
		BYTE i;
		extern BYTE i2c_delay_start /*= 160*/;
		extern BYTE i2c_delay_restart /*= 2*/;
		extern BYTE i2c_delay_datasetup /*= 32*/;
		extern BYTE i2c_delay_clockkhigh /*= 32*/;
		extern BYTE i2c_delay_datahold /*= 32*/;


		if(!stricmp(argv1[1],"?")) {
			Printf("\nSW i2c ");

		}
		if(!stricmp(argv[1],"reset")) {
			dbg_sw_i2c_sda_count = 0;
			dbg_sw_i2c_scl_count = 0;
			sw_i2c_regidx = 0;
			for(i=0; i < 4; i++) {
				dbg_sw_i2c_index[i] = 0;
				dbg_sw_i2c_devid[i] = 0;
				dbg_sw_i2c_regidx[i] = 0;
				dbg_sw_i2c_data[i] = 0; 
			}
			i2c_delay_start = 160;
			i2c_delay_restart = 2;
			i2c_delay_datasetup = 32;
			i2c_delay_clockkhigh = 32;
			i2c_delay_datahold = 32;
		}

#if 0
		if(!stricmp(argv[1],"delay_start")) {
			if( argc>=3 ) 
				i2c_delay_start = (BYTE)a2h( argv[2] );
		}
		if(!stricmp(argv[1],"delay_restart")) {
			if( argc>=3 ) 
				i2c_delay_restart = (BYTE)a2h( argv[2] );
		}
		if(!stricmp(argv[1],"delay_datasetup")) {
			if( argc>=3 ) 
				i2c_delay_datasetup = (BYTE)a2h( argv[2] );
		}
		if(!stricmp(argv[1],"delay_clockhigh")) {
			if( argc>=3 ) 
				i2c_delay_clockhigh = (BYTE)a2h( argv[2] );
		}
		if(!stricmp(argv[1],"delay_datahold")) {
			if( argc>=3 ) 
				i2c_delay_datahold = (BYTE)a2h( argv[2] );
		}
		Printf("\ni2c_delay start:%bx restart:%bx datasetup:%bx clockhigh:%bx datahold:%bx",
			i2c_delay_start,i2c_delay_restart,i2c_delay_datasetup, i2c_delay_clockkhigh,i2c_delay_datahold);  	
 #endif
