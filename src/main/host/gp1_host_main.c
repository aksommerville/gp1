#include "gp1_host_internal.h"
#include "io/gp1_io_clock.h"
#include <signal.h>

/* Signal handler: If there's more than one host (???) they share a signal handler.
 */

static volatile int gp1_host_sigc=0;

static void gp1_host_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++gp1_host_sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Cleanup.
 */
 
void gp1_host_cleanup(struct gp1_host *host) {
  gp1_audio_play(host->audio,0);
  
  gp1_renderer_del(host->renderer);
  gp1_vm_del(host->vm);
  gp1_video_del(host->video);
  gp1_audio_del(host->audio);
  if (host->inputv) {
    while (host->inputc-->0) gp1_input_del(host->inputv[host->inputc]);
    free(host->inputv);
  }
}

/* Initialize VM.
 */
 
static int gp1_host_init_vm(struct gp1_host *host) {
  struct gp1_vm_delegate delegate={
    .userdata=host,
    .render=_gp1_host_render,
    .store=_gp1_host_store,
    .load=_gp1_host_load,
    .http_request=_gp1_host_http_request,
    .ws_connect=_gp1_host_ws_connect,
    .ws_send=_gp1_host_ws_send,
  };
  if (!(host->vm=gp1_vm_new(&delegate))) {
    fprintf(stderr,"%s: Failed to create new VM.\n",host->config->exename);
    return -1;
  }
  if (gp1_vm_load_rom(host->vm,host->rom)<0) {
    fprintf(stderr,"%s: Failed to attach ROM file to VM.\n",host->config->exename);
    return -1;
  }
  if (gp1_vm_init_game(host->vm)<0) {
    fprintf(stderr,"%s: Failed to start VM.\n",host->config->exename);
    return -1;
  }
  return 0;
}

/* Select and initialize video driver.
 */
 
static int gp1_host_init_video_driver_1(struct gp1_host *host,const struct gp1_video_type *type) {
  struct gp1_video_delegate delegate={
    .userdata=host,
    .cb_close=_gp1_host_video_close,
    .cb_focus=_gp1_host_video_focus,
    .cb_resize=_gp1_host_video_resize,
    .cb_key=_gp1_host_video_key,
    .cb_text=_gp1_host_video_text,
    .cb_mmotion=_gp1_host_video_mmotion,
    .cb_mbutton=_gp1_host_video_mbutton,
    .cb_mwheel=_gp1_host_video_mwheel,
  };
  int w=host->rom->fbw;
  int h=host->rom->fbh;
  int rate=60;
  int fullscreen=0;//TODO configurable
  const char *title=host->rom->ht_title;
  int titlec=-1;
  const void *icon=0;//TODO window icon. can we put that in the ROM file? or use a pretty but generic GP1 one.
  int iconw=0,iconh=0;
  if (!(host->video=gp1_video_new(type,&delegate,w,h,rate,fullscreen,title,titlec,icon,iconw,iconh))) {
    return -1;
  }
  return 0;
}

static int gp1_host_init_renderer_1(struct gp1_host *host,const struct gp1_renderer_type *type) {
  if (!(host->renderer=gp1_renderer_new(type))) return -1;
  //TODO Do the video driver and renderer need to be introduced to each other?
  return 0;
}
  
static int gp1_host_init_video_driver(struct gp1_host *host) {
  if (host->config->video_driver_name) {
    const struct gp1_video_type *type=gp1_video_type_by_name(host->config->video_driver_name,-1);
    if (!type) {
      fprintf(stderr,"%s: Video driver '%s' not found.\n",host->config->exename,host->config->video_driver_name);
      return -1;
    }
    if (gp1_host_init_video_driver_1(host,type)<0) {
      fprintf(stderr,"%s: Failed to initialize video driver '%s'.\n",host->config->exename,type->name);
      return -1;
    }
  } else {
    int p=0; for (;;p++) {
      const struct gp1_video_type *type=gp1_video_type_by_index(p);
      if (!type) {
        fprintf(stderr,"%s: Failed to initialize any video driver.\n",host->config->exename);
        return -1;
      }
      if (gp1_host_init_video_driver_1(host,type)>=0) break;
    }
  }
  fprintf(stderr,"%s: Using video driver '%s'\n",host->config->exename,host->video->type->name);
  
  if (host->config->renderer_name) {
    const struct gp1_renderer_type *type=gp1_renderer_type_by_name(host->config->renderer_name,-1);
    if (!type) {
      fprintf(stderr,"%s: Renderer '%s' not found.\n",host->config->exename,host->config->renderer_name);
      return -1;
    }
    if (gp1_host_init_renderer_1(host,type)<0) {
      fprintf(stderr,"%s: Failed to initialize renderer '%s'.\n",host->config->exename,type->name);
      return -1;
    }
  } else {
    int p=0; for (;;p++) {
      const struct gp1_renderer_type *type=gp1_renderer_type_by_index(p);
      if (!type) {
        fprintf(stderr,"%s: Failed to initialize any renderer.\n",host->config->exename);
        return -1;
      }
      if (gp1_host_init_renderer_1(host,type)>=0) break;
    }
  }
  fprintf(stderr,"%s: Using renderer '%s'\n",host->config->exename,host->renderer->type->name);
  
  return 0;
}

/* Select and initialize audio driver.
 */
 
static int gp1_host_init_audio_driver_1(struct gp1_host *host,const struct gp1_audio_type *type) {
  struct gp1_audio_delegate delegate={
    .userdata=host,
    .cb_pcm_out=_gp1_host_audio_pcm_out,
  };
  if (!(host->audio=gp1_audio_new(type,&delegate,host->config->audio_rate,host->config->audio_chanc))) return -1;
  return 0;
}
  
static int gp1_host_init_audio_driver(struct gp1_host *host) {
  if (host->config->audio_driver_name) {
    const struct gp1_audio_type *type=gp1_audio_type_by_name(host->config->audio_driver_name,-1);
    if (!type) {
      fprintf(stderr,"%s: Audio driver '%s' not found.\n",host->config->exename,host->config->audio_driver_name);
      return -1;
    }
    if (gp1_host_init_audio_driver_1(host,type)<0) {
      fprintf(stderr,"%s: Failed to initialize audio driver '%s'.\n",host->config->exename,type->name);
      return -1;
    }
  } else {
    int p=0; for (;;p++) {
      const struct gp1_audio_type *type=gp1_audio_type_by_index(p);
      if (!type) {
        fprintf(stderr,"%s: Failed to initialize any audio driver.\n",host->config->exename);
        return -1;
      }
      if (gp1_host_init_audio_driver_1(host,type)>=0) break;
    }
  }
  fprintf(stderr,"%s: Using audio driver '%s', %d Hz, %d channels\n",host->config->exename,host->audio->type->name,host->audio->rate,host->audio->chanc);
  if ((host->audio->rate!=host->config->audio_rate)||(host->audio->chanc!=host->config->audio_chanc)) {
    fprintf(stderr,"%s: ...not exactly what was requested (%d Hz, %d channels)\n",host->config->exename,host->config->audio_rate,host->config->audio_chanc);
  }
  if (gp1_vm_init_synth(host->vm,host->audio->rate,host->audio->chanc)<0) {
    fprintf(stderr,"%s: Failed to initialize synthesizer.\n",host->config->exename);
    return -1;
  }
  if (gp1_audio_play(host->audio,1)<0) {
    fprintf(stderr,"%s: Failed to start audio.\n",host->config->exename);
    return -1;
  }
  return 0;
}

/* Select and initialize input drivers.
 */
 
static int gp1_host_init_input_driver_1(struct gp1_host *host,const struct gp1_input_type *type) {
  if (host->inputc>=host->inputa) {
    int na=host->inputa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(host->inputv,sizeof(void*)*na);
    if (!nv) return -1;
    host->inputv=nv;
    host->inputa=na;
  }
  struct gp1_input_delegate delegate={
    .userdata=host,
    .connect=_gp1_host_input_connect,
    .disconnect=_gp1_host_input_disconnect,
    .button=_gp1_host_input_button,
  };
  struct gp1_input *input=gp1_input_new(type,&delegate);
  if (!input) return -1;
  host->inputv[host->inputc++]=input;
  return 0;
}
  
static int gp1_host_init_input_drivers(struct gp1_host *host) {
  const char *src=host->config->input_driver_names;
  if (src) {
    int srcp=0;
    while (src[srcp]) {
      if (src[srcp]==',') { srcp++; continue; }
      const char *name=src+srcp;
      int namec=0;
      while (name[namec]&&(name[namec]!=',')) namec++;
      srcp+=namec;
      const struct gp1_input_type *type=gp1_input_type_by_name(name,namec);
      if (!type) {
        fprintf(stderr,"%s: Ignoring unknown input type '%.*s'\n",host->config->exename,namec,name);
      } else if (gp1_host_init_input_driver_1(host,type)<0) {
        fprintf(stderr,"%s: Failed to initialize input driver '%.*s', proceeding anyway.\n",host->config->exename,namec,name);
      } else {
        fprintf(stderr,"%s: Using input driver '%.*s'\n",host->config->exename,namec,name);
      }
    }
  } else {
    int p=0; for (;;p++) {
      const struct gp1_input_type *type=gp1_input_type_by_index(p);
      if (!type) break;
      if (gp1_host_init_input_driver_1(host,type)<0) {
        fprintf(stderr,"%s: Failed to initialize input driver '%s', proceeding anyway.\n",host->config->exename,type->name);
      } else {
        fprintf(stderr,"%s: Using input driver '%s'\n",host->config->exename,type->name);
      }
    }
  }
  return 0;
} 

/* Initialize drivers.
 */
 
static int gp1_host_init_drivers(struct gp1_host *host) {
  if (gp1_host_init_video_driver(host)<0) return -1;
  if (gp1_host_init_audio_driver(host)<0) return -1;
  if (gp1_host_init_input_drivers(host)<0) return -1;
  return 0;
}

/* Update drivers.
 * Log any errors.
 */
 
static int gp1_host_update_drivers(struct gp1_host *host) {
  if (gp1_video_update(host->video)<0) {
    fprintf(stderr,"%s: Failed to update video driver.\n",host->config->exename);
    return -1;
  }
  if (gp1_audio_update(host->audio)<0) {
    fprintf(stderr,"%s: Failed to update audio driver.\n",host->config->exename);
    return -1;
  }
  int i=host->inputc;
  while (i-->0) {
    struct gp1_input *input=host->inputv[i];
    if (gp1_input_update(input)<0) {
      fprintf(stderr,"%s: Failed to update input driver '%s'.\n",host->config->exename,input->type->name);
      return -1;
    }
  }
  return 0;
}

/* Run.
 */

int gp1_host_run(struct gp1_host *host) {

  signal(SIGINT,gp1_host_rcvsig);
  
  if (gp1_host_init_vm(host)<0) return -1;
  if (gp1_host_init_drivers(host)<0) return -1;
  
  double starttime_real=gp1_now_s();
  double starttime_cpu=gp1_now_cpu_s();
  double nexttime=starttime_real;
  double frametime=1.0/GP1_UPDATE_RATE;
  int framec=0;
  
  int err=0;
  while (1) {
  
    if (gp1_host_sigc) {
      gp1_host_sigc=0;
      fprintf(stderr,"%s: Terminate due to signal.\n",host->config->exename);
      break;
    }
    if (host->quit_requested) {
      fprintf(stderr,"%s: Terminate due to user request.\n",host->config->exename);
      break;
    }
    
    double now=gp1_now_s();
    if (now<nexttime) {
      while (now<nexttime) {
        int us=(nexttime-now)*1000000.0+10; // +10 to prevent it landing on the edge
        if (us>1000000) { // improbably long sleep time. reset the clock
          nexttime=now;
          host->clockfaultc++;
          break;
        }
        gp1_sleep(us);
        now=gp1_now_s();
      }
      nexttime+=frametime;
    } else {
      nexttime+=frametime;
      if (nexttime<now) { // far in the past. reset the clock
        nexttime=now;
        host->clockfaultc++;
      }
    }
    
    if ((err=gp1_host_update_drivers(host))<0) break;
    
    if ((err=gp1_vm_update(host->vm))<0) {
      fprintf(stderr,"%s: Error updating VM.\n",host->config->exename);
      break;
    }
    
    if ((err=gp1_video_swap(host->video))<0) {
      fprintf(stderr,"%s: Error delivering video frame.\n",host->config->exename);
      break;
    }
    framec++;
  }
  
  if ((err>=0)&&(framec>0)) {
    double endtime_real=gp1_now_s();
    double endtime_cpu=gp1_now_cpu_s();
    double elapsed_real=endtime_real-starttime_real;
    double elapsed_cpu=endtime_cpu-starttime_cpu;
    fprintf(stderr,
      "%s:%s: %d frames in %.0f s, average %.03f Hz. Average CPU usage %.06f\n",
      host->config->exename,host->rompath,
      framec,elapsed_real,framec/elapsed_real,elapsed_cpu/elapsed_real
    );
  }
  
  return err;
}
